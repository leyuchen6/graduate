#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int main ( void )
{
	int fd ;
	int retval ;
	char buf[2] ;
	fd = open ( "/dev/dht11_dev0" , O_RDONLY) ;
	if ( fd == -1 )
	{
		perror ( "open dht11_dev0 error\n" ) ;
		exit ( -1 ) ;
	}
	sleep ( 1 ) ;
	while ( 1 )
	{
		retval = read ( fd , buf ,2 );
		if ( retval < 0 )
		{
			perror ( "read dht11 error" ) ;
			printf ( "read dht11 error" ) ;
			continue;
		//	exit ( -1 ) ;
		}
		printf ( "\nHumidity:%d Temperature:%d \n", buf[0], buf[1]) ;
		printf("===================");
		sleep ( 3 ) ;
	}
	close ( fd ) ;
}

