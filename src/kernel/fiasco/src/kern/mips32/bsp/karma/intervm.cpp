INTERFACE:

#include "kobject_helper.h"
#include "hypercall.h"
#include "karma/karma_devices.h"


class Intervm : public Kobject_h<Intervm>
{
  FIASCO_DECLARE_KOBJ();
};

IMPLEMENTATION:


static void karma_intervm_write(unsigned long opcode, unsigned long val){
//        printk("[KARMA_CHR_INTERVM_PROD] %s\n", __func__);
     karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(shm_prod), opcode), &val);
 }
//
static void karma_intervm_write2(unsigned long opcode, unsigned long val, unsigned long val2){
//                //       printk("[KARMA_CHR_INTERVM_PROD] %s\n", __func__);
    karma_hypercall2(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(shm_prod), opcode), &val, &val2);
}


static unsigned long karma_intervm_read(unsigned long opcode){
    unsigned long ret = 0;  
//        printk("[KARMA_CHR_INTERVM_PROD] %s\n", __func__);
    karma_hypercall1(KARMA_MAKE_COMMAND(KARMA_DEVICE_ID(shm_prod), opcode), &ret); 
    return ret;
}

FIASCO_DEFINE_KOBJ(Intervm);

PUBLIC
Intervm::Intervm()
{
    initial_kobjects.register_obj(this, 9);    
    printf("Intervm kernel object created\n");
   // karma_intervm_write(99, 1);
}

PUBLIC void
Intervm::operator delete (void *)
{
  printf("WARNING: tried to delete kernel Intervm object.\n");
}


PUBLIC
L4_msg_tag
Intervm::kinvoke(L4_obj_ref ref, L4_fpage::Rights rights, Syscall_frame *f,
              Utcb const *r_msg, Utcb *s_msg)
{
   unsigned long address, address2;
   L4_msg_tag const t = f->tag();
   (void)ref;
   (void)rights;

   //printf("intervm kernel object kinvoke\n");
   switch (r_msg->values[0])
   {
       case 0:
           //printf("Intervm kernel object got message!\n");
           //printf("trying hypercall.....\n");

           address = (unsigned long)r_msg->values[2];

           //printf("address: 0x%08lx\n", address);

           karma_intervm_write(r_msg->values[1], address );
           return no_reply();
           break;
       case 1:

           address = (unsigned long)r_msg->values[2];
           address2 = (unsigned long)r_msg->values[3];

           //printf("address1: 0x%08lx address2: 0x%08lx\n", address, address2);

           karma_intervm_write2(r_msg->values[1], address, address2);
           return no_reply();
           break;
       
       case 2:

          if (!have_receive(s_msg))
              return commit_result(0);

          s_msg->values[2] = karma_intervm_read(r_msg->values[1]);
 
          return commit_result(0, 3);       
   }

   return no_reply();
}

static Intervm __intervm;
