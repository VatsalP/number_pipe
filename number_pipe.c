#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/slab.h>


#define DEVICE_NAME "numpipe"
#define CLASS_NAME "np"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vatsal Parekh");
MODULE_DESCRIPTION("Linux kernel module for numpipe");
MODULE_VERSION("0.1");


// Stuff
struct semaphore mut;
struct semaphore full;
struct semaphore empty;
static int majorNumber;
int * bufferQueue;
int bufferSize = 100;
static int currentSize = 0;
module_param(bufferSize, int, 0);

static int num_pipe_init_module(void);
static char * char_devnode(struct device *, umode_t *);
static void num_pipe_exit_module(void);
static int num_pipe_open(struct inode *, struct file *);
static int num_pipe_release(struct inode *, struct file *);
static ssize_t num_pipe_read(struct file *, char *, size_t, loff_t *);
static ssize_t num_pipe_write(struct file *, const char *, size_t, loff_t *);
static struct class *  numpipeClass = NULL;
static struct device * numpipeDevice = NULL;

// file operations of the character device
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = num_pipe_open,
    .read = num_pipe_read,
    .write = num_pipe_write,
    .release = num_pipe_release,
};

static int __init num_pipe_init_module(void) {
    // stuff to do before setting semaphores
    printk(KERN_INFO "NUMPIPE: INIT NUMPIPE\n");
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "NUMPIPE failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "NUMPIPE: registered major number\n");
    // Register the device class
    numpipeClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(numpipeClass)) {                // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "NUMPIPE: Failed to register device class\n");
        return PTR_ERR(numpipeClass);          // Correct way to return an error on a pointer
    }
    numpipeClass->devnode = char_devnode;
    printk(KERN_INFO "NUMPIPE: device class registered correctly\n");
    // Register the device driver
    numpipeDevice = device_create(numpipeClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(numpipeDevice)) {               // Clean up if there is an error
        class_destroy(numpipeClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "NUMPIPE: Failed to create the device\n");
        return PTR_ERR(numpipeDevice);
    }
    printk(KERN_INFO "NUMPIPE: device class created correctly\n"); // Made it! device was initialized
    // stuff done

    // Module config
    sema_init(&full, 0);
    sema_init(&empty, bufferSize);
    sema_init(&mut, 1);
    bufferQueue = kmalloc(bufferSize * sizeof(int), GFP_KERNEL); // Allocate Kernel ram

    return 0;
}

// this is to set permission of the file so anybody can read or write to it
static char * char_devnode(struct device * dev, umode_t * mode) {
    if (!mode)
        return NULL;
    if (dev->devt == MKDEV(majorNumber, 0))
        * mode = 0666;
    return NULL;
}

static void __exit num_pipe_exit_module(void) {
    device_destroy(numpipeClass, MKDEV(majorNumber, 0));
    class_unregister(numpipeClass);
    class_destroy(numpipeClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    kfree(bufferQueue);
    printk(KERN_INFO "NUMPIPE: DOBBY IS FREE!\n");
}

static int num_pipe_open(struct inode * inodep, struct file * filep) {
    printk(KERN_INFO "NUMPIPE: Opened\n");
    return 0;
}

static int num_pipe_release(struct inode * inodep, struct file * filep) {
    printk(KERN_INFO "NUMPIPE: Device succesfully closed\n");
    return 0;
}

static ssize_t num_pipe_read(struct file * filep, char * buffer, size_t len, loff_t * offset) {
    int i = 0;
    int couldnt_read;
    if (len != sizeof(int)) {
        printk(KERN_INFO "NUMPIPE: Can only read sizeof(int) no. of bytes\n");
        return -1;
    }
    if (down_interruptible(&full) != 0) { // Trying to remove one int
        printk(KERN_INFO "NUMPIPE: Process Interrupted\n"); // Got -EINTR from user
        return -1;
    }
    if (down_interruptible(&mut) != 0) { // Entering critical region
        printk(KERN_INFO "NUMPIPE: Process Interrupted\n"); // Got -EINTR from user
        return -1;
    }
    // Entered
    couldnt_read = copy_to_user(buffer, bufferQueue, sizeof(int));
    if (couldnt_read > 0) {
        printk(KERN_WARNING "NUMPIPE: Couldn't read using copy_to_user\n");
        return -1;
    }
    // Left Shift buffer now by one -- dequeued
    for (i = 0; i < currentSize - 1; i++) {
        bufferQueue[i] = bufferQueue[i+1];
    }
    currentSize -= 1;
    up(&mut); // Exiting critical region
    up(&empty); // One got read so empty by one more
    return sizeof(int); // if we reached here then sizeof(int) bytes are written into user buffer
}

static ssize_t num_pipe_write(struct file * filep, const char * buffer, size_t len, loff_t * offset) {
    int couldnt_write;
    if (len != sizeof(int)) {
        printk(KERN_INFO "NUMPIPE: Can only write sizeof(int) no. of bytes");
        return -1;
    }
    if (down_interruptible(&empty) != 0) { // Trying to remove one int
        printk(KERN_INFO "NUMPIPE: Process Interrupted"); // Got -EINTR from user
        return -1;
    }
    if (down_interruptible(&mut) != 0) { // Entering critical region
        printk(KERN_INFO "NUMPIPE: Process Interrupted"); // Got -EINTR from user
        return -1;
    }
    couldnt_write = copy_from_user(bufferQueue + currentSize, buffer, sizeof(int));
    if (couldnt_write > 0) {
        printk(KERN_WARNING "NUMPIPE: Couldn't read using copy_from_user");
        return -1;
    }
    currentSize += 1;
    up(&mut); // critical section is done now
    up(&full); // filled one more
    return sizeof(int);
}

module_init(num_pipe_init_module);
module_exit(num_pipe_exit_module);

