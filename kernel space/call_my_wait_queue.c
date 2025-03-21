#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <asm/current.h>
#include <linux/slab.h>//用來使用kmalloc
#include <linux/delay.h>
#include <linux/mutex.h>

struct my_data {
    int pid;
    struct list_head list;
};

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
LIST_HEAD(my_list);//這段代碼會定義並初始化一個名為 my_list 的鏈表頭
static DEFINE_MUTEX(my_mutex); 

static int condition = 0;  


static int enter_wait_queue(void){
    int pid = (int)current->pid;
    //current->pid這裡的資料型態為pid_t所以轉型為int
    
    
    struct my_data *entry=kmalloc(sizeof(*entry), GFP_KERNEL);
     // 分配記憶體儲存進程資訊
    /*
    也可以寫成
    struct my_data *entry=(struct my_data *)kmalloc(sizeof(struct my_data), GFP_KERNEL);
    
    在sizeof
    sizeof(*entry) 和 sizeof(struct my_data) 計算的結果相同，
    因為 *entry 指向的類型正是 struct my_data。
    
    1.kmalloc 的返回值是 void *，在user mode寫的malloc返回值也是 void *，
      即指向未定義類型的指標。
    2.在 C 語言中，void * 可以隱式轉換為任何其他類型的指標，
      例如 struct my_data *，不需要顯式轉型。
      
      在內核中為什麼不建議顯式轉型？
      掩蓋類型錯誤： 如果你錯誤地將返回值轉型為錯誤的類型，編譯器可能無法警告你。
      增加代碼冗餘： 由於 void * 的自動轉換，
      顯式轉型只是在多餘地重申一件內核中已經明確的事情。
    */
    
    
    printk("add to wait queue\n");
    entry->pid = pid;
    //把當前的process[(int)current->pid)]的pid存進當前process的my_data這個結構裡的pid
    list_add_tail(&entry->list, &my_list);
    //把當前的my_data加到自訂義雙向鏈結串列的尾部
    printk("Added process with pid=%d to my_list\n", entry->pid);
    wait_event_interruptible(my_wait_queue, condition == pid);
    //讓當前Process睡眠，並設立condition==pid這個喚醒條件，
    //等clean_wait_queue裡的wake_up_interruptible這函式喚醒
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
            mutex_lock(&my_mutex); //避免兩個process互相競爭造成race condition
            enter_wait_queue();
            mutex_unlock(&my_mutex);
            break;
        case 2:
            clean_wait_queue();
            break;
    }
    return 0;
}
