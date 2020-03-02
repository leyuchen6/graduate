
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

/* firstdrvtest on
  * firstdrvtest off
  */
int main(int argc, char **argv)
{
	int fd;
	unsigned char led_vals[2];
	
	fd = open("/dev/leds", O_RDWR);
	if (fd < 0)
		{
			printf("can't open!\n");
		}
	
    if (!strcmp("led1", argv[1])){
		led_vals[0] = 0; 
		
	}
	
    if (!strcmp("led2", argv[1])){
		led_vals[0] = 1; 
		
	}
    if (!strcmp("led2", argv[1])){
		led_vals[0] = 2; 
		
	}
    if (!strcmp("leds", argv[1])){
		led_vals[0] = 3; 
		
	}
	
    if (!strcmp("on", argv[2]))
    	{
			led_vals[1] = 0; 
		}
    if (!strcmp("off", argv[2]))
		{
			led_vals[1] = 1; 
		}


	printf("%d,%d\n",led_vals[0],led_vals[1]);
	write(fd, led_vals,2);
	return 0;
}
