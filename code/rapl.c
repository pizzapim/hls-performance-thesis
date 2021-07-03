/* Read the RAPL registers on recent (>sandybridge) Intel processors	*/
/*									*/
/* There are currently three ways to do this:				*/
/*	1. Read the MSRs directly with /dev/cpu/??/msr			*/
/*	2. Use the perf_event_open() interface				*/
/*	3. Read the values from the sysfs powercap interface		*/
/*									*/
/* MSR Code originally based on a (never made it upstream) linux-kernel	*/
/*	RAPL driver by Zhang Rui <rui.zhang@intel.com>			*/
/*	https://lkml.org/lkml/2011/5/26/93				*/
/* Additional contributions by:						*/
/*	Romain Dolbeau -- romain @ dolbeau.org				*/
/*									*/
/* For raw MSR access the /dev/cpu/??/msr driver must be enabled and	*/
/*	permissions set to allow read access.				*/
/*	You might need to "modprobe msr" before it will work.		*/
/*									*/
/* perf_event_open() support requires at least Linux 3.14 and to have	*/
/*	/proc/sys/kernel/perf_event_paranoid < 1			*/
/*									*/
/* the sysfs powercap interface got into the kernel in 			*/
/*	2d281d8196e38dd (3.13)						*/
/*									*/
/* Compile with:   gcc -O2 -Wall -o rapl-read rapl-read.c -lm		*/
/*									*/
/* Vince Weaver -- vincent.weaver @ maine.edu -- 11 September 2015	*/
/*									*/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <sys/syscall.h>

#define MAX_CPUS 1024
#define MAX_PACKAGES 16

static int package_map[MAX_PACKAGES];

static void detect_packages(int *total_cores, int *total_packages) {

  char filename[BUFSIZ];
  FILE *fff;
  int package;
  int i;

  *total_packages = 0;

  for (i = 0; i < MAX_PACKAGES; i++)
    package_map[i] = -1;

  fprintf(stderr, "\t");
  for (i = 0; i < MAX_CPUS; i++) {
    sprintf(filename,
            "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
    fff = fopen(filename, "r");
    if (fff == NULL)
      break;
    fscanf(fff, "%d", &package);
    fprintf(stderr, "%d (%d)", i, package);
    if (i % 8 == 7)
      fprintf(stderr, "\n\t");
    else
      fprintf(stderr, ", ");
    fclose(fff);

    if (package_map[package] == -1) {
      (*total_packages)++;
      package_map[package] = i;
    }
  }

  fprintf(stderr, "\n");

  *total_cores = i;

  fprintf(stderr, "\tDetected %d cores in %d packages\n\n", *total_cores,
          *total_packages);
}

#define NUM_RAPL_DOMAINS 5

int rapl_sysfs(void (*func)(void), double *result) {

  char event_names[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
  char filenames[MAX_PACKAGES][NUM_RAPL_DOMAINS][256];
  char basename[MAX_PACKAGES][256];
  char tempfile[256];
  long long before[MAX_PACKAGES][NUM_RAPL_DOMAINS];
  long long after[MAX_PACKAGES][NUM_RAPL_DOMAINS];
  int valid[MAX_PACKAGES][NUM_RAPL_DOMAINS];
  int i, j;
  FILE *fff;

  int total_cores, total_packages;
  detect_packages(&total_cores, &total_packages);

  fprintf(stderr, "\nTrying sysfs powercap interface to gather results\n\n");

  for (j = 0; j < total_packages; j++) {
    i = 0;
    sprintf(basename[j], "/sys/class/powercap/intel-rapl/intel-rapl:%d", j);
    sprintf(tempfile, "%s/name", basename[j]);
    fff = fopen(tempfile, "r");
    if (fff == NULL) {
      fprintf(stderr, "\tCould not open %s\n", tempfile);
      return -1;
    }
    fscanf(fff, "%s", event_names[j][i]);
    valid[j][i] = 1;
    fclose(fff);
    sprintf(filenames[j][i], "%s/energy_uj", basename[j]);

    /* Handle subdomains */
    for (i = 1; i < NUM_RAPL_DOMAINS; i++) {
      sprintf(tempfile, "%s/intel-rapl:%d:%d/name", basename[j], j, i - 1);
      fff = fopen(tempfile, "r");
      if (fff == NULL) {
        valid[j][i] = 0;
        continue;
      }
      valid[j][i] = 1;
      fscanf(fff, "%s", event_names[j][i]);
      fclose(fff);
      sprintf(filenames[j][i], "%s/intel-rapl:%d:%d/energy_uj", basename[j], j,
              i - 1);
    }
  }

  /* Gather before values */
  for (j = 0; j < total_packages; j++) {
    for (i = 0; i < NUM_RAPL_DOMAINS; i++) {
      if (valid[j][i]) {
        fff = fopen(filenames[j][i], "r");
        if (fff == NULL) {
          fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
        } else {
          fscanf(fff, "%lld", &before[j][i]);
          fclose(fff);
        }
      }
    }
  }

  func();

  /* Gather after values */
  for (j = 0; j < total_packages; j++) {
    for (i = 0; i < NUM_RAPL_DOMAINS; i++) {
      if (valid[j][i]) {
        fff = fopen(filenames[j][i], "r");
        if (fff == NULL) {
          fprintf(stderr, "\tError opening %s!\n", filenames[j][i]);
        } else {
          fscanf(fff, "%lld", &after[j][i]);
          fclose(fff);
        }
      }
    }
  }

  const char *package_prefix = "package-";
  double total_joules = 0.;

  for (j = 0; j < total_packages; j++) {
    for (i = 0; i < NUM_RAPL_DOMAINS; i++) {
      if (valid[j][i]) {
        if (strncmp(event_names[j][i], package_prefix,
                    strlen(package_prefix)) == 0) {
          total_joules += (double)after[j][i] - (double)before[j][i];
        }
      }
    }
  }

  *result = total_joules;

  return 0;
}
