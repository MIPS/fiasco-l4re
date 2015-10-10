IMPLEMENTATION [mips32]:

EXTENSION class Jdb_tcb
{
  enum
  {
    Disasm_x = 41,
    Disasm_y = 11,
    Stack_y  = 20,
  };
};

IMPLEMENT
Address Jdb_tcb_ptr::entry_ip() const
{
  return top_value(-4);
}

IMPLEMENT
void Jdb_tcb::print_entry_frame_regs(Thread *t)
{
  Jdb_entry_frame *ef = Jdb::get_entry_frame(Jdb::current_cpu);
  int from_user       = ef->from_user();
  Mem_space *s = t->mem_space();

  printf("\n\n\nRegisters (before debug entry from %s mode):\n"
         "[0]  %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[8]  %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[16] %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[24] %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "SR=%08lx EPC=%08lx CAUSE=%08lx BADVADDR=%08lx USP=%08lx asid=%lx\n",
         from_user ? "user" : "kernel",
    ef->r[0],  ef->r[1], ef->r[2], ef->r[3], ef->r[4],
    ef->r[5],  ef->r[6], ef->r[7], ef->r[8], ef->r[9],
    ef->r[10], ef->r[11], ef->r[12], ef->r[13], ef->r[14],
    ef->r[15], ef->r[16], ef->r[17], ef->r[18], ef->r[19],
    ef->r[20], ef->r[21], ef->r[22], ef->r[23], ef->r[24],
    ef->r[25], ef->r[26], ef->r[27], ef->r[28], ef->r[29],
    ef->r[30], ef->r[31],
    ef->status, ef->epc, ef->cause, ef->badvaddr, ef->usp, s->c_asid());
}


IMPLEMENT
void
Jdb_tcb::info_thread_state(Thread *t)
{
  Mem_space *s = t->mem_space();

  Jdb_tcb_ptr current((Address)t->get_kernel_sp());

  printf("\n\n\nSR=%s%08lx\033[m USP=%08lx ENTRY IP=%08lx asid=%lx\n",
      Jdb::esc_emph, current.top_value(-5), current.top_value(-1),
      current.top_value(-4), s->c_asid());

  printf("[0]  %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[8]  %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[16] %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "[24] %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx"
         "\033[m\n",
         current.top_value(-39), //0
         current.top_value(-38), //1
         current.top_value(-37), //2
         current.top_value(-36), //3
         current.top_value(-35), //4
         current.top_value(-34), //5
         current.top_value(-33), //6
         current.top_value(-32), //7
         current.top_value(-31), //8
         current.top_value(-30), //9
         current.top_value(-29), //10
         current.top_value(-28), //11
         current.top_value(-27), //12
         current.top_value(-26), //13
         current.top_value(-25), //14
         current.top_value(-24), //15
         current.top_value(-23), //16
         current.top_value(-22), //17
         current.top_value(-21), //18
         current.top_value(-20), //19
         current.top_value(-19), //20
         current.top_value(-18), //21
         current.top_value(-17), //22
         current.top_value(-16), //23
         current.top_value(-15), //24
         current.top_value(-14), //25
         current.top_value(-13), //26
         current.top_value(-12), //27
         current.top_value(-11), //28
         current.top_value(-10), //29
         current.top_value(-9), //30
         current.top_value(-8) //31
        );
}

IMPLEMENT
void
Jdb_tcb::print_return_frame_regs(Jdb_tcb_ptr const &, Address)
{}

IMPLEMENT
bool
Jdb_stack_view::edit_registers()
{
  return false;
}

IMPLEMENT inline
bool
Jdb_tcb_ptr::is_user_value() const
{
  return _offs >= Context::Size - sizeof(Return_frame);
}

IMPLEMENT inline
const char *
Jdb_tcb_ptr::user_value_desc() const
{
  const char *desc[] = { "USP", "BADVADDR", "CAUSE",
                         "EPC", "STATUS", "HI", "LO"};
  return desc[(Context::Size - _offs) / sizeof(Mword) - 1];
}
