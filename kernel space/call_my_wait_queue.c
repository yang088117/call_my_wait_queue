#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <asm/current.h>
#include <linux/slab.h>//�ΨӨϥ�kmalloc
#include <linux/delay.h>
#include <linux/mutex.h>

struct my_data {
    int pid;
    struct list_head list;
};

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
LIST_HEAD(my_list);//�o�q�N�X�|�w�q�ê�l�Ƥ@�ӦW�� my_list ������Y
static DEFINE_MUTEX(my_mutex); 

static int condition = 0;  


static int enter_wait_queue(void){
    int pid = (int)current->pid;
    //current->pid�o�̪���ƫ��A��pid_t�ҥH�૬��int
    
    
    struct my_data *entry=kmalloc(sizeof(*entry), GFP_KERNEL);
     // ���t�O�����x�s�i�{��T
    /*
    �]�i�H�g��
    struct my_data *entry=(struct my_data *)kmalloc(sizeof(struct my_data), GFP_KERNEL);
    
    �bsizeof
    sizeof(*entry) �M sizeof(struct my_data) �p�⪺���G�ۦP�A
    �]�� *entry ���V���������O struct my_data�C
    
    1.kmalloc ����^�ȬO void *�A�buser mode�g��malloc��^�Ȥ]�O void *�A
      �Y���V���w�q���������СC
    2.�b C �y�����Avoid * �i�H�����ഫ�������L���������СA
      �Ҧp struct my_data *�A���ݭn�㦡�૬�C
      
      �b���֤������򤣫�ĳ�㦡�૬�H
      ���\�������~�G �p�G�A���~�a�N��^���૬�����~�������A�sĶ���i��L�kĵ�i�A�C
      �W�[�N�X���l�G �ѩ� void * ���۰��ഫ�A
      �㦡�૬�u�O�b�h�l�a���Ӥ@�󤺮֤��w�g���T���Ʊ��C
    */
    
    
    printk("add to wait queue\n");
    entry->pid = pid;
    //���e��process[(int)current->pid)]��pid�s�i��eprocess��my_data�o�ӵ��c�̪�pid
    list_add_tail(&entry->list, &my_list);
    //���e��my_data�[��ۭq�q���V�쵲��C������
    printk("Added process with pid=%d to my_list\n", entry->pid);
    wait_event_interruptible(my_wait_queue, condition == pid);
    //����eProcess�ίv�A�ó]��condition==pid�o�ӳ������A
    //��clean_wait_queue�̪�wake_up_interruptible�o�禡���
    return 0;
}
static int clean_wait_queue(void){
    struct my_data *entry;
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;
        printk("wake up pid=%d\n", condition);
        wake_up_interruptible(&my_wait_queue); 
        msleep(100);
    }

    return 0;
}
SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            mutex_lock(&my_mutex); //�קK���process�����v���y��race condition
            enter_wait_queue();
            mutex_unlock(&my_mutex);
            break;
        case 2:
            clean_wait_queue();
            break;
    }
    return 0;
}
