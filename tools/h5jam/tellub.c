/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "hdf5.h"
#include "h5tools_utils.h"

#define TRUE 1
#define FALSE 0

const char *progname = "tellub";

/*
 * Command-line options: The user can specify short or long-named
 * parameters. The long-named ones can be partially spelled. When
 * adding more, make sure that they don't clash with each other.
 */
static const char *s_opts = "h";
static struct long_options l_opts[] = {
  {"help", no_arg, 'h'},
  {"hel", no_arg, 'h'},
  {NULL, 0, '\0'}
};

/*-------------------------------------------------------------------------
 * Function:    usage
 *
 * Purpose:     Print the usage message 
 *
 * Return:      void
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void
usage (const char *prog)
{
  fflush (stdout);
  fprintf (stdout, "usage: %s h5_file\n", prog);
  fprintf (stdout,
	   "           Check that h5_fil is HDF5 file and print size of user block \n");
  fprintf (stdout, "       %s -h\n", prog);
  fprintf (stdout, "           Print a usage message and exit\n");
}

/*-------------------------------------------------------------------------
 * Function:    parse_command_line
 *
 * Purpose:     Parse the command line for the h5dumper.
 *
 * Return:      Success:    
 *
 *              Failure:    Exits program with EXIT_FAILURE value.
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

void
parse_command_line (int argc, const char *argv[])
{
  int opt = FALSE;

  /* parse command line options */
  while ((opt = get_option (argc, argv, s_opts, l_opts)) != EOF)
    {
      switch ((char) opt)
	{
	case 'h':
	  usage (progname);
	  exit (EXIT_SUCCESS);
	case '?':
	default:
	  usage (progname);
	  exit (EXIT_FAILURE);
	}
    }

  /* check for file name to be processed */
  if (argc <= opt_ind)
    {
      error_msg (progname, "missing file name\n");
      usage (progname);
      exit (EXIT_FAILURE);
    }
}

/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     HDF5 user block unjammer
 *
 * Return:      Success:    0
 *              Failure:    1
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
main (int argc, const char *argv[])
{
  char *ifname;
  void *edata;
  H5E_auto_t func;
  hid_t ifile;
  hsize_t usize;
  htri_t testval;
  herr_t status;
  hid_t plist;

  /* Disable error reporting */
#ifdef H5_WANT_H5_V1_6_COMPAT
  H5Eget_auto(&func, &edata);
  H5Eset_auto(NULL, NULL);
#else /* H5_WANT_H5_V1_6_COMPAT */
  H5Eget_auto (H5E_DEFAULT, &func, &edata);
  H5Eset_auto (H5E_DEFAULT, NULL, NULL);
#endif /* H5_WANT_H5_V1_6_COMPAT */

  parse_command_line (argc, argv);

  if (argc <= (opt_ind))
    {
      error_msg (progname, "missing file name\n");
      usage (progname);
      exit (EXIT_FAILURE);
    }

  ifname = strdup (argv[opt_ind]);

  testval = H5Fis_hdf5 (ifname);

  if (testval <= 0)
    {
      error_msg (progname, "Input HDF5 file is not HDF \"%s\"\n", ifname);
      exit (EXIT_FAILURE);
    }

  ifile = H5Fopen (ifname, H5F_ACC_RDONLY, H5P_DEFAULT);

  if (ifile < 0)
    {
      error_msg (progname, "Can't open input HDF5 file \"%s\"\n", ifname);
      exit (EXIT_FAILURE);
    }

  plist = H5Fget_create_plist (ifile);
  if (plist < 0)
    {
      error_msg (progname, "Can't get file creation plist for file \"%s\"\n",
		 ifname);
      exit (EXIT_FAILURE);
    }

  status = H5Pget_userblock (plist, &usize);
  if (status < 0)
    {
      error_msg (progname, "Can't get user block for file \"%s\"\n", ifname);
      exit (EXIT_FAILURE);
    }

  printf ("%ld\n", (long) usize);

  H5Pclose (plist);
  H5Fclose (ifile);

  exit (EXIT_SUCCESS);
}
