#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/wait.h>

//working with an array

static size_t size_arr = 100;

struct circular_array
{
	char *arr;
	size_t size;
	ssize_t iread;
	ssize_t iwrite;
	ssize_t distance;
	struct mutex lock;
	wait_queue_head_t q;
};

static struct circular_array *initer_buf(ssize_t size)
{
	struct circular_array *temp;
	temp = kmalloc(sizeof(struct circular_array), GFP_KERNEL);
	if(temp == NULL)
		return NULL;
	temp->arr = kmalloc(size, GFP_KERNEL);
	if(temp->arr == NULL)
	{
		kfree(temp);
		return NULL;
	}
	temp->size = size;
	temp->iread = 0;
	temp->iwrite = 0;
	temp->distance = 0;
	mutex_init(&temp->lock);
	init_waitqueue_head(&temp->q);
	return temp;
}

struct circular_array *cbuf;

static void exit_buf(struct circular_array *buf)
{
	kfree(buf->arr);
	kfree(buf);
}

//main
static int major;

static ssize_t kanal_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	char *tbuf = kmalloc(count, GFP_KERNEL);
	size_t cr = 0;
	int flag;
	if (tbuf == NULL)
	{
		pr_err("Could not allocate tbuf in read\n");
		return -1;
	}
	flag = mutex_lock_interruptible(&cbuf->lock);
	if (flag != 0)
	{
		pr_err("Mutex interrupted with return value %d\n", flag);
		return cr;
	}
	while(cr < count)
	{
		if(cbuf->distance > 0)
		{
			tbuf[cr++] = cbuf->arr[cbuf->iread++];
			printk("read %c\n",tbuf[cr-1]);
			cbuf->distance--;
			if(cbuf->iread >= cbuf->size)
				cbuf->iread = 0;
			if(cr == count)
				break;
			wake_up(&cbuf->q);
			mutex_unlock(&cbuf->lock);
			flag = wait_event_interruptible(cbuf->q, cbuf->distance < cbuf->size);
			if (flag == -ERESTARTSYS)
			{
				pr_err("Sleep interrupted with return value %d\n", flag);
				return cr;
			}
			flag = mutex_lock_interruptible(&cbuf->lock);
			if (flag != 0)
			{
				pr_err("Mutex interrupted with return value %d\n", flag);
				return cr;
			}
		}
		else
		{
			flag = wait_event_interruptible(cbuf->q, cbuf->distance < cbuf->size);
			if (flag == -ERESTARTSYS)
			{
				pr_err("Sleep interrupted with return value %d\n", flag);
				return cr;
			}
			flag = mutex_lock_interruptible(&cbuf->lock);
			if (flag != 0)
			{
				pr_err("Mutex interrupted with return value %d\n", flag);
				return cr;
			}
		}
	}
	
	wake_up(&cbuf->q);
	mutex_unlock(&cbuf->lock);

	unsigned long copy = copy_to_user(buf, tbuf, count);
	
	if (copy != 0)
	{
		pr_err("Couldn't copy buffer from user in read\n");
		pr_err("Returning 0 to user.\n");
		return 0;
	}
	
	kfree(tbuf);

	return cr;
}

static ssize_t kanal_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	char *tbuf = kmalloc(count, GFP_KERNEL);
	unsigned long copy = copy_from_user(tbuf, buf, count);
	size_t cw = 0;
	int flag;

	if (tbuf == NULL)
	{
		pr_err("Could not allocate tbuf in write\n");
		return -1;
	}
	if (copy != 0)
	{
		pr_err("Couldn't copy buffer from user in write\n");
		pr_err("Returning 0 to user.\n");
		return 0;
	}
	flag = mutex_lock_interruptible(&cbuf->lock);
	if (flag != 0)
	{
		pr_err("Mutex interrupted with return value %d\n", flag);
		return cw;
	}

	while(cw < count)
	{
		if(cbuf->distance < cbuf->size)
		{
			cbuf->arr[cbuf->iwrite++] = tbuf[cw++];
			printk("write %c\n",cbuf->arr[cbuf->iwrite-1]);
			cbuf->distance++;
			if(cbuf->iwrite >= cbuf->size)
				cbuf->iwrite = 0;
			if(cw == count)
				break;
			wake_up(&cbuf->q);
			mutex_unlock(&cbuf->lock);
			flag = wait_event_interruptible(cbuf->q, cbuf->distance > 0);
			if (flag == -ERESTARTSYS)
			{
				pr_err("Sleep interrupted with return value %d\n", flag);
				return cw;
			}
			flag = mutex_lock_interruptible(&cbuf->lock);
			if (flag != 0)
			{
				pr_err("Mutex interrupted with return value %d\n", flag);
				return cw;
			}
		}
		else
		{
			flag = wait_event_interruptible(cbuf->q, cbuf->distance > 0);
			if (flag == -ERESTARTSYS)
			{
				pr_err("Sleep interrupted with return value %d\n", flag);
				return cw;
			}
			flag = mutex_lock_interruptible(&cbuf->lock);
			if (flag != 0)
			{
				pr_err("Mutex interrupted with return value %d\n", flag);
				return cw;
			}
		}
	}

	wake_up(&cbuf->q);
	mutex_unlock(&cbuf->lock);

	kfree(tbuf);

	return cw;
}



int kanal_open(struct inode *in, struct file *fl)
{
	cbuf = initer_buf(size_arr);
	printk("Just open\n");
	return 0;
}

int kanal_close(struct inode *in, struct file *fl)
{
	exit_buf(cbuf);
	printk("Just close\n");
	return 0;
}

static struct file_operations fops =
{
	.read = kanal_read,
	.write = kanal_write,
	.open = kanal_open,
	.release = kanal_close
};

static int __init modinit(void)
{
	major = register_chrdev(0, "kanal", &fops);
	if (major < 0)
	{
		printk("failed %d\n", major);
		return major;
	}
	printk("/dev/kanal assigned major %d\n", major);
	return 0;
}

static void __exit modexit(void)
{
	unregister_chrdev(major, "register_chrdev");
}

module_init(modinit);
module_exit(modexit);
MODULE_AUTHOR("Vlasov and Dzubko");
MODULE_DESCRIPTION("register_chrdev test");
MODULE_LICENSE("GPL");
