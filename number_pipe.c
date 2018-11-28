#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "numpipe"
#define CLASS_NAME "np"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vatsal Parekh");
MODULE_DESCRIPTION("Linux kernel module for numpipe");
MODULE_VERSION("0.1");


// Stuff
static int majorNumber;
static int bufferSize = 100;
struct semaphore mutex;
struct semaphore full;
struct semaphore empty;
static DEFINE_SEMAPHORE(full);
static DEFINE_SEMAPHORE(empty);
static DEFINE_MUTEX(mutex);

sema_init(&empty, bufferSize);
sema_init(&full, 0);

int init_module(void);
void cleanup_module(void);
static int num_pipe_open(struct inode *, struct file *);
static int num_pipe_release(struct inode *, struct file *);
static ssize_t num_pipe_read(struct file *, char *, size_t, loff_t *);
static ssize_t num_pipe_write(struct file *, const char *, size_t, loff_t *);
static struct class *  numpipeClass = NULL;
static struct device * numpipeDevice = NULL;

static struct file_operations fops = {
    .open = num_pipe_open,
    .read = num_pipe_read,
    .write = num_pipe_write,
    .release = num_pipe_release,
};

static int __init init_module(void) {
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
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(numpipeClass);          // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "NUMPIPE: device class registered correctly\n");
    // Register the device driver
    numpipeDevice = device_create(numpipeClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(numpipeDevice)){               // Clean up if there is an error
        class_destroy(numpipeClass);           // Repeated code but the alternative is goto statements
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(numpipeDevice);
    }
    printk(KERN_INFO "NUMPIPE: device class created correctly\n"); // Made it! device was initialized
    return 0;
}

static void __exit cleanup_module(void) {
    device_destroy(numpipeClass, MKDEV(majorNumber, 0));
    class_unregister(numpipeClass);
    class_destroy(numpipeClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO)
}

static int num_pipe_release(struct inode * inodep, struct file * filep) {
    printk(KERN_INFO "NUMPIPE: Device succesfully closed\n");
    return 0;
}

static ssize_t num_pipe_read(struct file * filep, char * buffer, size_t len, loff_t * offset) {
    return 0;
}

static ssize_t num_pipe_write(struct file * filep, char * buffer, size_t len, loff_t * offset) {
    return 0;
}