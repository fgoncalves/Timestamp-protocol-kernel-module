#ifndef __STATISTICS_H__
#define __STATISTICS_H__
extern void write_data(unsigned long long tstamp, unsigned long long delay, unsigned int id, char* source, unsigned char* collected_data);
extern void close_statistics();
extern void init_statistics();
extern char* get_statistics_file();
#endif
