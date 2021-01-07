/**************************************************************************//**

  @file     SddpServer.c
  @brief    Provides an example SDDP application
  @section  Platform Platform(s)
			Linux
  @section  Detail Detail
            This application demonstrations the usage of SDDP APIs provided
            by sddp.c/h. This file is intended to be utilized on linux
            platforms but can be ported to others, or provide a good starting
            point for an example. 
            
            Use the '-n' commandline option to run in the foreground (no daemon),
            or no commandline options to start as daemon. '-h' will provide
            usage. 

******************************************************************************/
///////////////////////////////////////
// Includes
///////////////////////////////////////
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <ctype.h>
#include <dirent.h>

#include "Sddp.h"

///////////////////////////////////////
// Defines
///////////////////////////////////////
#define DAEMON_NAME    		"sddpd"
#define CONFIG_FILE		"/etc/sddpd.conf"


///////////////////////////////////////
// Typedefs
///////////////////////////////////////


///////////////////////////////////////
// Variables
///////////////////////////////////////
static volatile int identify_signal = 0;
static volatile int termination_signal = 0;
static char config_file[256] = CONFIG_FILE;

///////////////////////////////////////
// Function Prototypes
///////////////////////////////////////
static int HandleArgs(int argc, char *argv[], const SDDPHandle handle);
static void SignalHandler(int iSignal);
static void daemonize();
static void PrintUsage(int argc, char *argv[]);
static int ReadSddpConfig(const SDDPHandle handle, const char *configFile);
static char *trimRight(char *str, char *end);
static char *skipWhitespace(char *str);

/*****************************************************************************

 Function Name: main

 Description: Program entry

 Parameters:  argc - Standard argument count
			  argv - Standard argument array

 Returns:     int status code

*****************************************************************************/
int main(int argc, char *argv[])
{
	SDDPHandle handle;
	char host[MAX_HOST_SIZE];
	int daemon;

	// Setup logging
    openlog(DAEMON_NAME, LOG_CONS | LOG_PID, LOG_LOCAL0);

	// Set up SDDP library
	SDDPInit(&handle);

    // Handle command line arguments, load config files as needed
	daemon = HandleArgs(argc, argv, handle);

	// daemonize if required, daemonStart will cause the starting process to
        // exit and the child will continue below
	if (daemon)
		daemonize();

	// Setup signal handling. USR1 sends IDENTIFY message
	signal(SIGTERM, SignalHandler);
	signal(SIGUSR1, SignalHandler);

	// Initialize the SDDP host name
	if(gethostname(host, MAX_HOST_SIZE) == 0)
		SDDPSetHost(handle, host);
	else
		SDDPSetHost(handle, "unknown");

	syslog(LOG_INFO, "Device Host %s\n", host);

	// Initialize SDDP device info
	int ret = ReadSddpConfig(handle, config_file);
	if (!ret)
	{
		SDDPShutdown(&handle);
		fprintf(stderr, "Incomplete config file `%s'\n", config_file);
		exit(EXIT_FAILURE);
	}

	// Start up SDDP
	SDDPStart(handle);

	// Run SDDP loop
	while(!termination_signal)
	{
		// Call SDDP tick to process incoming and outgoing SDDP messages
		SDDPTick(handle);

		if (identify_signal)
		{
			// Send identify
			syslog(LOG_INFO, "Sending SDDP identify\n");

			SDDPIdentify(handle);

			identify_signal = 0;
		}

		usleep(1000); // Don't hog the CPU
	}

	// Shutdown SDDP
	SDDPShutdown(&handle);

	// Cleanup
	closelog();
	exit(EXIT_SUCCESS);

} // End of main

/*****************************************************************************

 Function Name: HandleArgs

 Description: Handles command line arguments

 Parameters:  argc - Standard argument count
			  argv - Standard argument array
			  handle - SDDP API handle

 Returns:     1 if we should run as daemon

*****************************************************************************/
int HandleArgs(int argc, char *argv[], const SDDPHandle handle)
{
	int daemon = 1;
	char c;

	// Check provided arguments
	while((c = getopt(argc, argv, "nhp:c:d:|help")) != -1) 
	{
		switch(c)
		{
            case 'h':
                PrintUsage(argc, argv);
                exit(EXIT_SUCCESS);
                break;
				
            case 'n':
                daemon = 0;
                break;
            
            case 'c':
            	strcpy(config_file, optarg);
            	break;
            default:
                PrintUsage(argc, argv);
                exit(EXIT_SUCCESS);
                break;
        }
    }
	
	return daemon;
}

void daemonize()
{
	pid_t pid, sid;

	syslog(LOG_INFO, "Daemonizing");
	// Fork off the parent process
	pid = fork();
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}

	// If we got a good PID, then exit the parent process
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	// Change the file mode mask
	umask(0);

	// Create a new SID for the child process
	sid = setsid();
	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}

	// Change the current working directory
	if ((chdir("/")) < 0)
	{
		exit(EXIT_FAILURE);
	}
}

/*****************************************************************************

 Function Name: SignalHandler

 Description: Handles signals

 Parameters:  iSignal - signal

 Returns:     void

*****************************************************************************/
static void SignalHandler(int iSignal)
{
	if (iSignal == SIGUSR1)
	{
		syslog(LOG_INFO, "Received button push event signal\n");
		
		identify_signal = 1;
	}
	else if (iSignal == SIGTERM || iSignal == SIGINT)
	{
		if (!termination_signal)
		{
			termination_signal = 1;
			syslog(LOG_INFO, "Terminating...\n");
		}
	}
}

/*****************************************************************************

 Function Name: PrintUsage

 Description: Prints comandline options

 Parameters:  argc - Standard argument count
			  argv - Standard argument array

 Returns:     void

*****************************************************************************/
static void PrintUsage(int argc, char *argv[]) 
{
    if (argc >=1) 
	{
        printf("Usage: %s -n\n", argv[0]);
        printf("  Options:\n");
        printf("      -n\tDon't start as daemon\n");
        printf("      -c FILE\tLoad the config file FILE.  If this argument is not specified, \n");
        printf("             \tit defaults to %s\n", CONFIG_FILE);
    }
}

/*****************************************************************************

 Function Name: ReadSddpConfig

 Description: Reads the configuration file

 Parameters:  handle - the SDDP API handle
              configFile - File name of the configuration file

 Returns:     int

*****************************************************************************/
int ReadSddpConfig(SDDPHandle handle, const char *configFile)
{
	char line[256];
	char product_name[100], primary_proxy[100], proxies[256],
            manufacturer[100], model[100], driver[100];
	int max_age;
	char *key, *sep, *val;
	FILE *file;
	
	file = fopen(configFile, "r");
	if (file)
	{
		while (fgets(line, sizeof(line), file))
		{
			key = skipWhitespace(line);
			if (!key || key[0] == ';')
				continue;
			
			sep = strchr(key, '=');
			if (!sep)
				continue;
			
			key = trimRight(key, sep);
			if (!key)
				continue;
			
			val = skipWhitespace(sep + 1);
			if (val)
				val = trimRight(val, val + strlen(val));
			
			if (strcmp(key, "Type") == 0)
				strcpy(product_name, val);
			else if (strcmp(key, "PrimaryProxy") == 0)
				strcpy(primary_proxy, val);
			else if (strcmp(key, "Proxies") == 0)
				strcpy(proxies, val);
			else if (strcmp(key, "Manufacturer") == 0)
				strcpy(manufacturer, val);
			else if (strcmp(key, "Model") == 0)
				strcpy(model, val);
			else if (strcmp(key, "Driver") == 0)
				strcpy(driver, val);
			else if (strcmp(key, "MaxAge") == 0)
				max_age = val ? atoi(val) : 1800;
		}
		
		fclose(file);

		SDDPSetDevice(handle, 0, product_name, primary_proxy, proxies,
                              manufacturer, model, driver, max_age, false);
		return 1;
	}
	
	return 0;
}

static char *trimRight(char *str, char *end)
{
	if (end == str)
		return NULL;
	
	while (end > str)
	{
		if (!isspace(*(end - 1)))
			break;
		end--;
	}
	
	*end = '\0';
	return str;
}

static char *skipWhitespace(char *str)
{
	while (*str && isspace(*str))
		str++;
	return str;
}

