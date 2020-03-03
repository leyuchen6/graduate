#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>



int main(int argc, char **argv)  
{  
	int fd;  
	int ret;  
	int data;     
    fd = open("/dev/mq_2_dev0", 0);  
    if(fd < 0)  
    {  
        printf("Open MQ_2_ADC Device Faild!n");  
        exit(1);  
    }  
 
	sleep ( 3 ) ;
	while ( 1 )
	{
		ret = read(fd, &data, sizeof(data)); 
		if ( ret < 0 )
		{
			perror ( "read mq_2 error" ) ;
			printf ( "read mq_2 error" ) ;
			continue;
		
		}
		printf("Read ADC value  is: %d\n", data);
		printf("===================");
		sleep ( 3 ) ;
	}
    close(fd);  
    return 0;  
}


