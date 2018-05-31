/* 
 * mt.c: implementation of mt(1) as part of a Cygwin environment
 *
 * Copyright 1998,1999,2000,2001,2005,2009,2013  Corinna Vinschen,
 * bug reports to  cygwin@cygwin.com
 *
 * Caution: This Programm runs only under Cygwin since it uses
 *	    Cygwin specific struct mtget members.
 *
 *	    Remote devices are NOT supported!
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <mntent.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mtio.h>
#include <sys/cygwin.h>
#include <sys/utsname.h>

#include <windows.h>

static char *SCCSid = "@(#)mt V" VERSION ", Corinna Vinschen, " __DATE__ "\n";

char *myname;

struct mt_tape_info mt_tape_info[] = MT_TAPE_INFO;

struct 
{
  int code;
  char *name;
} densities[] =
{
  {0x00, "default"},
  {0x01, "NRZI (800 bpi)"},
  {0x02, "PE (1600 bpi)"},
  {0x03, "GCR (6250 bpi)"},
  {0x04, "QIC-11"},
  {0x05, "QIC-45/60 (GCR, 8000 bpi)"},
  {0x06, "PE (3200 bpi)"},
  {0x07, "IMFM (6400 bpi)"},
  {0x08, "GCR (8000 bpi)"},
  {0x09, "GCR (37871 bpi)"},
  {0x0a, "MFM (6667 bpi)"},
  {0x0b, "PE (1600 bpi)"},
  {0x0c, "GCR (12960 bpi)"},
  {0x0d, "GCR (25380 bpi)"},
  {0x0f, "QIC-120 (GCR 10000 bpi)"},
  {0x10, "QIC-150/250 (GCR 10000 bpi)"},
  {0x11, "QIC-320/525 (GCR 16000 bpi)"},
  {0x12, "QIC-1350 (RLL 51667 bpi)"},
  {0x13, "DDS (61000 bpi)"},
  {0x14, "EXB-8200 (RLL 43245 bpi)"},
  {0x15, "EXB-8500 or QIC-1000"},
  {0x16, "MFM 10000 bpi"},
  {0x17, "MFM 42500 bpi"},
  {0x18, "TZ86"},
  {0x19, "DLT 10GB"},
  {0x1a, "DLT 20GB"},
  {0x1b, "DLT 35GB"},
  {0x1c, "QIC-385M"},
  {0x1d, "QIC-410M"},
  {0x1e, "QIC-1000C"},
  {0x1f, "QIC-2100C"},
  {0x20, "QIC-6GB"},
  {0x21, "QIC-20GB (SLR 32)"},
  {0x22, "QIC-2GB"},
  {0x23, "QIC-875"},
  {0x24, "DDS-2"},
  {0x25, "DDS-3"},
  {0x26, "DDS-4 or QIC-4GB-DC (SLR 5)"},
  {0x27, "Exabyte Mammoth"},
  {0x28, "Exabyte Mammoth-2"},
  {0x29, "QIC-3080MC"},
  {0x30, "AIT-1 or MLR3 (SLR 50)"},
  {0x31, "AIT-2"},
  {0x32, "AIT-3 or ALRF-2 (SLR 7)"},
  {0x33, "AIT-4 or SLR6 (SLR 24)"},
  {0x34, "ALRF-1 (SLR 40/60/75/100)"},
  {0x36, "ALRF-6 (SLR 140)"},
  {0x3c, "LTO-3 WORM"},
  {0x40, "DLT1 40 GB, or Ultrium"},
  {0x41, "DLT 40 GB, or Ultrium2"},
  {0x42, "LTO-2"},
  {0x44, "LTO-3"},
  {0x45, "QIC-3095-MC (TR-4)"},
  {0x46, "LTO-4"},
  {0x47, "DDS-5 or TR-5"},
  {0x48, "Quantum SDLT220"},
  {0x49, "Quantum SDLT320"},
  {0x4a, "Quantum SDLT600 or STK T10KA"},
  {0x4b, "STK T10KB"},
  {0x4c, "LTO-4 WORM or STK T10KC"},
  {0x51, "IBM 3592 J1A"},
  {0x52, "IBM 3592 E05"},
  {0x53, "IBM 3592 E06"},
  {0x58, "LTO-5"},
  {0x5a, "LTO-6"},
  {0x5c, "LTO-5 WORM"},
  {0x71, "IBM 3592 J1A encrypted"},
  {0x72, "IBM 3592 E05 encrypted"},
  {0x73, "IBM 3592 E06 encrypted"},
  {0x80, "DLT 15GB uncomp. or Ecrix"},
  {0x81, "DLT 15GB or EXB-V23 comp."},
  {0x82, "DLT 20GB uncompressed"},
  {0x83, "DLT 20GB compressed"},
  {0x84, "DLT 35GB uncompressed"},
  {0x85, "DLT 35GB compressed"},
  {0x86, "DLT1 40 GB uncompressed"},
  {0x87, "DLT1 40 GB compressed"},
  {0x88, "DLT 40GB uncompressed"},
  {0x89, "DLT 40GB compressed"},
  {0x8c, "EXB-8505 compressed"},
  {0x90, "SDLT110 uncompr/EXB-8205 compr"},
  {0x91, "SDLT110 compressed"},
  {0x92, "SDLT160 uncompressed"},
  {0x93, "SDLT160 compressed"},
  {0, NULL}
};

int
errprintf (int ret, char *format, ...)
{
  va_list ap;

  fprintf (stderr, "%s: ", myname);
  va_start (ap, format);
  vfprintf (stderr, format, ap);
  va_end (ap);
  putc ('\n', stderr);
  return ret;
}

static int 
check_cygwin_api_version (int major, int minor)
{
  struct utsname uts;
  char *c;
  int api_major = 0, api_minor = 0;

  if (!uname(&uts) && (c = strchr(uts.release, '(')) != NULL)
    sscanf(c + 1, "%d.%d", &api_major, &api_minor);
  if (api_major < major
      || (api_major == major && api_minor < minor))
    return -1;
  if (api_major == major && api_minor == minor)
    return 0;
  return 1;
}

struct {
  DWORD value;
  char *str;
} feat[] = {
  { TAPE_DRIVE_FIXED_BLOCK,		"fixed length blocks" },
  { TAPE_DRIVE_VARIABLE_BLOCK,		"var length blocks" },
  { TAPE_DRIVE_SET_BLOCK_SIZE,		"set block size" },

  { TAPE_DRIVE_FIXED,			"fixed partitioning" },
  { TAPE_DRIVE_SELECT,			"select partitioning" },
  { TAPE_DRIVE_INITIATOR,		"initiator partitioning" },

  { TAPE_DRIVE_GET_LOGICAL_BLK,		"get logical blockno" },
  { TAPE_DRIVE_LOGICAL_BLK,		"logical block spacing" },
  { TAPE_DRIVE_GET_ABSOLUTE_BLK,	"get absolute blockno" },
  { TAPE_DRIVE_ABSOLUTE_BLK,		"absolute block spacing" },
  { TAPE_DRIVE_LOG_BLK_IMMED,		"logical block immediate" },
  { TAPE_DRIVE_ABS_BLK_IMMED,		"absolute block immediate" },

  { TAPE_DRIVE_RELATIVE_BLKS,		"relative block spacing" },
  { TAPE_DRIVE_REVERSE_POSITION,	"backward spacing" },

  { TAPE_DRIVE_REWIND_IMMEDIATE,	"immediate rewind" },
  { TAPE_DRIVE_END_OF_DATA,		"fast EOM spacing" },

  { TAPE_DRIVE_SPACE_IMMEDIATE,		"immediate spacing" },
  { TAPE_DRIVE_WRITE_MARK_IMMED,	"write marks immediate" },

  { TAPE_DRIVE_FILEMARKS,		"filemark spacing" },
  { TAPE_DRIVE_WRITE_FILEMARKS,		"write filemarks" },
  { TAPE_DRIVE_WRITE_LONG_FMKS,		"write long filemarks" },
  { TAPE_DRIVE_WRITE_SHORT_FMKS,	"write short filemarks" },

  { TAPE_DRIVE_REPORT_SMKS,		"report setmarks" },
  { TAPE_DRIVE_SETMARKS,		"setmark spacing" },
  { TAPE_DRIVE_WRITE_SETMARKS,		"write setmarks" },
  { TAPE_DRIVE_SET_REPORT_SMKS,		"set report setmarks" },

  { TAPE_DRIVE_SEQUENTIAL_FMKS,		"sequential filemarks" },
  { TAPE_DRIVE_SEQUENTIAL_SMKS,		"sequential setmarks" },

  { TAPE_DRIVE_LOAD_UNLOAD,		"load and unload" },
  { TAPE_DRIVE_LOAD_UNLD_IMMED,		"un/load immediate" },

  { TAPE_DRIVE_LOCK_UNLOCK,		"lock and unlock" },
  { TAPE_DRIVE_LOCK_UNLK_IMMED,		"un/lock immediate" },

  { TAPE_DRIVE_TENSION,			"tape retension" },
  { TAPE_DRIVE_TENSION_IMMED,		"retension immediate" },

  { TAPE_DRIVE_ERASE_BOP_ONLY,		"erase on bop only" },
  { TAPE_DRIVE_ERASE_IMMEDIATE,		"erase immediately" },
  { TAPE_DRIVE_ERASE_LONG,		"long erase" },
  { TAPE_DRIVE_ERASE_SHORT,		"short erase" },

  { TAPE_DRIVE_EJECT_MEDIA,		"sw ejects media" },
  { TAPE_DRIVE_WRITE_PROTECT,		"report write protection" },

  { TAPE_DRIVE_EOT_WZ_SIZE,		"report EOT warn size" },
  { TAPE_DRIVE_SET_EOT_WZ_SIZE,		"set EOT warn size" },

  { TAPE_DRIVE_PADDING,			"data padding" },
  { TAPE_DRIVE_SET_PADDING,		"set data padding" },

  { TAPE_DRIVE_TAPE_CAPACITY,		"returns capacity" },
  { TAPE_DRIVE_TAPE_REMAINING,		"returns remaining" },

  { TAPE_DRIVE_ECC,			"hw error correction" },
  { TAPE_DRIVE_SET_ECC,			"set hw error correction" },

  { TAPE_DRIVE_COMPRESSION,		"hw compression" },
  { TAPE_DRIVE_SET_COMPRESSION,		"set compression" },
  { TAPE_DRIVE_SET_CMP_BOP_ONLY,	"set comp. on bop only" },
  { TAPE_DRIVE_CLEAN_REQUESTS,		"report cleaning request" },

  { 0,					NULL }
};

static const char *
get_feature (int i, struct mtget *get)
{
  int ret = ((feat[i].value & TAPE_DRIVE_HIGH_FEATURES)
	     ? ((get->mt_featureshigh & feat[i].value) != 0)
	     : ((get->mt_featureslow & feat[i].value) != 0));
  return ret ? "yes" : "no ";
}

int
tape_status (int fd, struct mtget *get, long type)
{
  int i;

  int has_tape = !GMT_DR_OPEN (get->mt_gstat);

  if (!has_tape)
    {
      printf ("No media\n");
      if (type < 2)
	type = 2;
    }

  if (has_tape)
    {
      struct mtpos pos;
      int i;
      char *ti_name = "Generic";
      for (i = 0; mt_tape_info[i].t_name; ++i)
        if (get->mt_type == mt_tape_info[i].t_type)
	  {
	    ti_name = mt_tape_info[i].t_name;
	    break;
	  }
      if (type <= 1)
        {
	  int gnu_style = !!getenv ("MT_STATUS_GNU");
	  int ds = (get->mt_dsreg & MT_ST_DENSITY_MASK) >> MT_ST_DENSITY_SHIFT;
	  int i;
	  char *ds_name = "unknown";
	  if (gnu_style)
	    {
	      printf ("drive type = %s\n", ti_name);
	      printf ("drive status = %lu\n", get->mt_dsreg);
	      printf ("sense key error = %ld\n", get->mt_erreg);
	      printf ("residue count = %ld\n", get->mt_resid);
	      printf ("file number = %ld\n", get->mt_fileno);
	      printf ("block number = %ld\n", get->mt_blkno);
	    }
	  else
	    {
	      printf ("%s tape drive:\n", ti_name);
	      printf ("File number=%ld, block number=%ld, partition=%lu\n",
		      get->mt_fileno, get->mt_blkno, get->mt_resid & 0xffff);
	    }
	  printf ("Tape block size %lu bytes. ",
		  (get->mt_dsreg & MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT);
	  for (i = 0; densities[i].name; ++i)
	    if (ds == densities[i].code)
	      {
		ds_name = densities[i].name;
		break;
	      }
	  printf ("Density code 0x%02lx (%s).\n",
		  (get->mt_dsreg & MT_ST_DENSITY_MASK) >> MT_ST_DENSITY_SHIFT,
		  ds_name);
	  if (!gnu_style)
	    printf ("Soft error count since last status=%ld\n", get->mt_erreg);
	}
      else
	{
	  printf ("drive type       :       %02lx (%s)\n",
		  get->mt_type, ti_name);
	  if (get->mt_featureslow & TAPE_DRIVE_TAPE_CAPACITY)
	    printf ("tape capacity    : %8llu KB          ",
		    get->mt_capacity >> 10);
	  if (get->mt_featureslow & TAPE_DRIVE_TAPE_REMAINING)
	    printf ("remaining        : %8llu KB\n", get->mt_remaining >> 10);
	  else if (get->mt_featureslow & TAPE_DRIVE_TAPE_CAPACITY)
	    putc ('\n', stdout);
	    printf ("current file     : %8ld             ", get->mt_fileno);
	  printf ("active partition : %8lu", get->mt_resid & 0xffff);
	  if (check_cygwin_api_version (0, 270) >= 0
	      && (get->mt_resid >> 16) > 1)
	    printf (" (of %lu)", get->mt_resid >> 16);
	  puts ("");
	  printf ("current block    : %8ld             ", get->mt_blkno);
	  if (!ioctl (fd, MTIOCPOS, &pos))
	    printf ("cur logical block: %8ld", pos.mt_blkno);
	  puts ("");
        }
    }
  printf("General status bits on (%lx):\n", get->mt_gstat);
  if (GMT_EOF (get->mt_gstat))
    printf (" EOF");
  if (GMT_BOT (get->mt_gstat))
    printf (" BOT");
  if (GMT_EOT (get->mt_gstat))
    printf (" EOT");
  if (GMT_SM (get->mt_gstat))
    printf (" SM");
  if (GMT_EOD (get->mt_gstat))
    printf (" EOD");
  if (GMT_WR_PROT (get->mt_gstat))
    printf (" WR_PROT");
  if (GMT_ONLINE (get->mt_gstat))
    printf (" ONLINE");
  if (GMT_D_6250 (get->mt_gstat))
    printf (" D_6250");
  if (GMT_D_1600 (get->mt_gstat))
    printf (" D_1600");
  if (GMT_D_800 (get->mt_gstat))
    printf (" D_800");
  if (GMT_DR_OPEN (get->mt_gstat))
    printf (" DR_OPEN");       
  if (GMT_IM_REP_EN (get->mt_gstat))
    printf (" IM_REP_EN");
  if (type > 1)
    {
      if (GMT_CLN (get->mt_gstat))
	printf (" CLN");
      if (GMT_REP_SM (get->mt_gstat))
	printf (" REP_SM");
      if (GMT_PADDING  (get->mt_gstat))
	printf (" PADDING");
      if (GMT_HW_ECC  (get->mt_gstat))
	printf (" HW_ECC");
      if (GMT_HW_COMP  (get->mt_gstat))
	printf (" HW_COMP");
      if (GMT_TWO_FM  (get->mt_gstat))
	printf (" TWO_FM");
      if (GMT_FAST_MTEOM  (get->mt_gstat))
	printf (" FAST_MTEOM");
      if (GMT_AUTO_LOCK  (get->mt_gstat))
	printf (" AUTO_LOCK");
      if (GMT_SYSV  (get->mt_gstat))
	printf (" SYSV");
      if (GMT_NOWAIT  (get->mt_gstat))
	printf (" NOWAIT");
      if (GMT_ASYNC  (get->mt_gstat))
	printf (" ASYNC");
    }
  puts ("");
  if (type > 1)
    {
      if (get->mt_featureshigh & TAPE_DRIVE_SET_BLOCK_SIZE)
	{
	  printf
	    ("min block size   : %8u             max block size   : %8u\n",
	     get->mt_minblksize, get->mt_maxblksize);
	  printf ("def block size   : %8u             ",
		  get->mt_defblksize);
	}
      if (has_tape)
	printf ("cur block size   : %8lu\n",
		(get->mt_dsreg & MT_ST_BLKSIZE_MASK) >> MT_ST_BLKSIZE_SHIFT);
      else if (get->mt_featureshigh & TAPE_DRIVE_SET_BLOCK_SIZE)
	putc ('\n', stdout);
      if (has_tape)
        {
	  int ds = (get->mt_dsreg & MT_ST_DENSITY_MASK) >> MT_ST_DENSITY_SHIFT;
	  char *ds_name = "unknown";
	  int i;
	  for (i = 0; densities[i].name; ++i)
	    if (ds == densities[i].code)
	      {
	        ds_name = densities[i].name;
		break;
	      }
	  printf ("density code     :       %02lx (%s)\n",
		  (get->mt_dsreg & MT_ST_DENSITY_MASK) >> MT_ST_DENSITY_SHIFT,
		  ds_name);
        }
      if (get->mt_featureslow & TAPE_DRIVE_EOT_WZ_SIZE)
        printf ("EOT zone size    : %8lu\n", get->mt_eotwarningzonesize);
      else
	puts ("");
    }

  if (type > 2)
    {
      printf ("Drive(r) Capabilities:\n");
      printf ("----------------------\n");
      for (i = 0; feat[i].value; ++i)
	printf ("%-27.27s: %s%s", feat[i].str, get_feature (i, get),
		i % 2 ? "\n" : "        ");
    }
  return NO_ERROR;
}

int
usage ()
{
  fprintf (stderr, "usage: %s [-V] [-f device] operation [count]\n", myname);
  return 1;
}

int
main (int argc, char *argv[])
{
  char *cmd;
  char *path = NULL;
  long count = 1;
  int c;
  int ret = 0;
  int fd;
  struct stat st;
  struct mtop op;
  struct mtpos pos;

  myname = argv[0];

  setvbuf (stdout, NULL, _IONBF, 0);

  while ((c = getopt (argc, argv, "f:V")) != EOF)
    switch (c)
      {
      case 'f':
	if (!optarg)
	  {
	    usage ();
	    return 1;
	  }
	path = optarg;
	break;
      case 'V':
	fprintf (stderr, "%s", SCCSid + 4);
	return 0;
      case '?':
	return usage ();
      }

  if (optind >= argc)
    return usage ();
  cmd = argv[optind++];
  if (optind < argc && (count = strtol (argv[optind], NULL, 0)) < 0)
    return errprintf (1, "count must be positive.");

  if (!path)
    path = getenv ("TAPE");
  if (!path)
    path = strdup (DEFTAPE);

  if ((fd = open (path, O_RDWR)) < 0)
    return errprintf (1, "%s: %s", path, strerror (errno));

  if (fstat (fd, &st))
    return errprintf (1, "%s: %s", path, strerror (errno));

  if (!S_ISCHR (st.st_mode))
    return errprintf (1, "%s is not a character special file", path);

  op.mt_count = count;

  /* get status of tape */
  if (!strcmp (cmd, "status"))
    {
      struct mtget get;
      if (ioctl (fd, MTIOCGET, &get) < 0 && errno != ENOMEDIUM)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
      else
        tape_status (fd, &get, count);
      return ret;
    }
  /* write count EOF marks */
  else if (!strcmp (cmd, "eof") || !strcmp (cmd, "weof"))
    {
      op.mt_op = MTWEOF;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* forward count files */
  else if (!strcmp (cmd, "fsf"))
    {
      op.mt_op = MTFSF;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* backward count files */
  else if (!strcmp (cmd, "bsf"))
    {
      op.mt_op = MTBSF;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* forward count records */
  else if (!strcmp (cmd, "fsr"))
    {
      op.mt_op = MTFSR;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* backward count records */
  else if (!strcmp (cmd, "bsr"))
    {
      op.mt_op = MTBSR;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* forward count filemarks */
  else if (!strcmp (cmd, "fsfm"))
    {
      op.mt_op = MTFSFM;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* backward count filemarks */
  else if (!strcmp (cmd, "bsfm"))
    {
      op.mt_op = MTBSFM;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* move to absolut file number count */
  else if (!strcmp (cmd, "asf"))
    {
      op.mt_op = MTREW;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
      else
        {
	  op.mt_op = MTFSF;
	  op.mt_count = count;
	  if (ioctl (fd, MTIOCTOP, &op) < 0)
	    ret = errprintf (2, "%s: %s", path, strerror (errno));
	}
    }
  /* move to end of data */
  else if (!strcmp (cmd, "eom"))
    {
      op.mt_op = MTEOM;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* rewind tape */
  else if (!strcmp (cmd, "rewind"))
    {
      op.mt_op = MTREW;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* offline tape */
  else if (!strcmp (cmd, "rewoffl") || !strcmp (cmd, "offline"))
    {
      op.mt_op = MTOFFL;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* retension tape */
  else if (!strcmp (cmd, "retension"))
    {
      op.mt_op = MTREW;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
      else
        {
	  op.mt_op = MTRETEN;
	  if (ioctl (fd, MTIOCTOP, &op) < 0)
	    ret = errprintf (2, "%s: %s", path, strerror (errno));
	  else
	    {
	      op.mt_op = MTREW;
	      if (ioctl (fd, MTIOCTOP, &op) < 0)
		ret = errprintf (2, "%s: %s", path, strerror (errno));
	    }
	}
    }
  /* erase tape */
  else if (!strcmp (cmd, "erase"))
    {
      op.mt_op = MTERASE;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* forward count setmarks */
  else if (!strcmp (cmd, "fss"))
    {
      op.mt_op = MTFSS;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* backward count setmarks */
  else if (!strcmp (cmd, "bss"))
    {
      op.mt_op = MTBSS;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* write count setmarks */
  else if (!strcmp (cmd, "wset"))
    {
      op.mt_op = MTWSM;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* move to end of data */
  else if (!strcmp (cmd, "eod") || !strcmp (cmd, "seod"))
    {
      op.mt_op = MTEOM;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* set block size */
  else if (!strcmp (cmd, "setblk"))
    {
      op.mt_op = MTSETBLK;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* set density */
  else if (!strcmp (cmd, "setdensity"))
    {
      op.mt_op = MTSETDENSITY;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* set drive buffer size or flags */
  else if (!strcmp (cmd, "drvbuffer"))
    {
      op.mt_op = MTSETDRVBUFFER;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* set driver options */
  else if (!strcmp (cmd, "stoptions"))
    {
      op.mt_op = MTSETDRVBUFFER;
      op.mt_count &= ~MT_ST_OPTIONS;
      op.mt_count |= MT_ST_BOOLEANS;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* set write threshold */
  else if (!strcmp (cmd, "stwrthreshold"))
    {
      ret = errprintf (1, "operation '%s' not implemented", cmd);
    }
  /* seek to count block */
  else if (!strcmp (cmd, "seek"))
    {
      op.mt_op = MTSEEK;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  /* tell block count */
  else if (!strcmp (cmd, "tell"))
    {
      /* Don't use MTTELL call.  It's non-standarized and returns the block
         number as an int, which breaks 64 bit block numbers on 64 bit systems.
	 Unfortunately, following the Linux definitions, the return type in
	 struct mtpos is long, which restricts it to 32 bit on 32 bit systems
	 as well. */
      if (ioctl (fd, MTIOCPOS, &pos) < 0)
	ret = errprintf (2, "%s: %s", path, strerror (errno));
      else
	printf ("At block %d.\n", pos.mt_blkno);
    }
  /* write densities */
  else if (!strcmp (cmd, "densities"))
    {
      puts ("Some SCSI tape density codes:\n"
	    "code   explanation");
      for (count = 0; densities[count].name; ++count)
        printf ("0x%02x   %s\n", densities[count].code, densities[count].name);
    }
  /* get/set compression */
  else if (!strcmp (cmd, "datcompression")	/* GNU mt compatibility. */
	   || !strcmp (cmd, "compression"))	/* Fedora mt compatibility. */
    {
      struct mtget get;

      if (count != 1)
        {
	  op.mt_op = MTCOMPRESSION;
	  if (ioctl (fd, MTIOCTOP, &op) < 0)
	    ret = errprintf (2, "%s: %s", path, strerror (errno));
	}
      if (!ioctl (fd, MTIOCGET, &get) || errno == ENOMEDIUM)
	printf ("Compression %s.\n", GMT_HW_COMP (get.mt_gstat)
				     ? "on" : "off");
    }
  else if (!strcmp (cmd, "mkpart")		/* Backward compatibility. */
	   || !strcmp (cmd, "mkpartition"))	/* Fedora mt compatibility. */
    {
      op.mt_op = MTMKPART;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
        ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  else if (!strcmp (cmd, "setpart")		/* Backward compatibility. */
	   || !strcmp (cmd, "setpartition"))	/* Fedora mt compatibility. */
    {
      op.mt_op = MTSETPART;
      if (ioctl (fd, MTIOCTOP, &op) < 0)
        ret = errprintf (2, "%s: %s", path, strerror (errno));
    }
  else if (!strcmp (cmd, "partseek"))
    {
      op.mt_op = MTSETPART;
      op.mt_count = (optind + 1 < argc ? atoi (argv[optind + 1]) : 0);
      if (ioctl (fd, MTIOCTOP, &op) < 0)
      	ret = errprintf (2, "%s: %s", path, strerror (errno));
      else
	{
	  op.mt_op = MTSEEK;
	  op.mt_count = count;
	  if (ioctl (fd, MTIOCTOP, &op) < 0)
	    ret = errprintf (2, "%s: %s", path, strerror (errno));
	}
    }
  else
    ret = errprintf (1, "unknown operation '%s'", cmd);

  close (fd);
  return ret;
}
