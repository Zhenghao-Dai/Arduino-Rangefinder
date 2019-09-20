void init_timer0(unsigned short m);
void init_buzzer(void);
void serial_init(unsigned short);
void serial_stringout(char *);
void serial_txchar(char);
void serial_sender(int distances);
void clear_distance();
void clear_raw();
void display (char distance[]);
void sender(int distances);
void receiver();
int serial_distance;
int serial_flag;//if changed