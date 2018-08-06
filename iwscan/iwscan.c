/***********************************************
* iwscan
* adapted from Jean Tourrilhes' iwlist
* m@matte.io
*
* This file is released under the GPL license.
*
***********************************************/

#include "iwlib-private.h"		/* Private header */
#include <sys/time.h>

/****************************** TYPES ******************************/

#define MAX_NUM_RESULTS 32

struct iw_ap
{
  char			essid[4*IW_ESSID_MAX_SIZE+1];
  char 			bssid[12];
  int8_t		strength;  
};

struct iw_results 
{
  uint8_t		num_of_ap;
  struct iw_ap		aps[MAX_NUM_RESULTS];  
};
///

/**************************** CONSTANTS ****************************/

#define IW_SCAN_HACK		0x8000

#define IW_EXTKEY_SIZE	(sizeof(struct iw_encode_ext) + IW_ENCODING_TOKEN_MAX)

/* ------------------------ WPA CAPA NAMES ------------------------ */
/*
 * This is the user readable name of a bunch of WPA constants in wireless.h
 * Maybe this should go in iwlib.c ?
 */

#ifndef WE_ESSENTIAL
#define IW_ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))

#ifndef IW_IE_CIPHER_NONE
/* Cypher values in GENIE (pairwise and group) */
#define IW_IE_CIPHER_NONE	0
#define IW_IE_CIPHER_WEP40	1
#define IW_IE_CIPHER_TKIP	2
#define IW_IE_CIPHER_WRAP	3
#define IW_IE_CIPHER_CCMP	4
#define IW_IE_CIPHER_WEP104	5
/* Key management in GENIE */
#define IW_IE_KEY_MGMT_NONE	0
#define IW_IE_KEY_MGMT_802_1X	1
#define IW_IE_KEY_MGMT_PSK	2
#endif	/* IW_IE_CIPHER_NONE */

/* Values for the IW_IE_CIPHER_* in GENIE */
static const char *	iw_ie_cypher_name[] = {
	"none",
	"WEP-40",
	"TKIP",
	"WRAP",
	"CCMP",
	"WEP-104",
};
#define	IW_IE_CYPHER_NUM	IW_ARRAY_LEN(iw_ie_cypher_name)

/* Values for the IW_IE_KEY_MGMT_* in GENIE */
static const char *	iw_ie_key_mgmt_name[] = {
	"none",
	"802.1x",
	"PSK",
};
#define	IW_IE_KEY_MGMT_NUM	IW_ARRAY_LEN(iw_ie_key_mgmt_name)

#endif	/* WE_ESSENTIAL */


void iws_ether_ntop(const struct ether_addr * eth, char* ethaddr)
{
  sprintf(ethaddr, "%02X%02X%02X%02X%02X%02X",
          eth->ether_addr_octet[0], eth->ether_addr_octet[1],
          eth->ether_addr_octet[2], eth->ether_addr_octet[3],
          eth->ether_addr_octet[4], eth->ether_addr_octet[5]);
}


void iw_sether_ntop(const struct sockaddr *sap, char* ethaddr)
{
  iws_ether_ntop((const struct ether_addr *) sap->sa_data, ethaddr);;
}


/************************* WPA SUBROUTINES *************************/

#ifndef WE_ESSENTIAL

/*------------------------------------------------------------------*/
/*
 * Print the name corresponding to a value, with overflow check.
 */
static void
iw_print_value_name(unsigned int		value,
		    const char *		names[],
		    const unsigned int		num_names)
{
  if(value >= num_names)
    printf(" unknown (%d)", value);
  else
    printf(" %s", names[value]);
}

/*------------------------------------------------------------------*/
/*
 * Parse, and display the results of an unknown IE.
 *
 */
static void 
iw_print_ie_unknown(unsigned char *	iebuf,
		    int			buflen)
{
  int	ielen = iebuf[1] + 2;
  int	i;

  if(ielen > buflen)
    ielen = buflen;

  printf("Unknown: ");
  for(i = 0; i < ielen; i++)
    printf("%02X", iebuf[i]);
  printf("\n");
}

/*------------------------------------------------------------------*/
/*
 * Parse, and display the results of a WPA or WPA2 IE.
 *
 */
static inline void 
iw_print_ie_wpa(unsigned char *	iebuf,
		int		buflen)
{
  int			ielen = iebuf[1] + 2;
  int			offset = 2;	/* Skip the IE id, and the length. */
  unsigned char		wpa1_oui[3] = {0x00, 0x50, 0xf2};
  unsigned char		wpa2_oui[3] = {0x00, 0x0f, 0xac};
  unsigned char *	wpa_oui;
  int			i;
  uint16_t		ver = 0;
  uint16_t		cnt = 0;

  if(ielen > buflen)
    ielen = buflen;

  switch(iebuf[0])
    {
    case 0x30:		/* WPA2 */
      /* Check if we have enough data */
      if(ielen < 4)
	{
	  iw_print_ie_unknown(iebuf, buflen);
 	  return;
	}

      wpa_oui = wpa2_oui;
      break;

    case 0xdd:		/* WPA or else */
      wpa_oui = wpa1_oui;
 
      /* Not all IEs that start with 0xdd are WPA. 
       * So check that the OUI is valid. Note : offset==2 */
      if((ielen < 8)
	 || (memcmp(&iebuf[offset], wpa_oui, 3) != 0)
	 || (iebuf[offset + 3] != 0x01))
 	{
	  iw_print_ie_unknown(iebuf, buflen);
 	  return;
 	}

      /* Skip the OUI type */
      offset += 4;
      break;

    default:
      return;
    }
  
  /* Pick version number (little endian) */
  ver = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;

  if(iebuf[0] == 0xdd)
    printf("WPA Version %d\n", ver);
  if(iebuf[0] == 0x30)
    printf("IEEE 802.11i/WPA2 Version %d\n", ver);

  /* From here, everything is technically optional. */

  /* Check if we are done */
  if(ielen < (offset + 4))
    {
      /* We have a short IE.  So we should assume TKIP/TKIP. */
      printf("Group Cipher : TKIP\n");
      printf("Pairwise Cipher : TKIP\n");
      return;
    }
 
  /* Next we have our group cipher. */
  if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
    {
      printf("Group Cipher : Proprietary\n");
    }
  else
    {
      printf("Group Cipher :");
      iw_print_value_name(iebuf[offset+3],
			  iw_ie_cypher_name, IW_IE_CYPHER_NUM);
      printf("\n");
    }
  offset += 4;

  /* Check if we are done */
  if(ielen < (offset + 2))
    {
      /* We don't have a pairwise cipher, or auth method. Assume TKIP. */
      printf("Pairwise Ciphers : TKIP\n");
      return;
    }

  /* Otherwise, we have some number of pairwise ciphers. */
  cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;
  printf("Pairwise Ciphers (%d) :", cnt);

  if(ielen < (offset + 4*cnt))
    return;

  for(i = 0; i < cnt; i++)
    {
      if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
 	{
 	  printf(" Proprietary");
 	}
      else
	{
	  iw_print_value_name(iebuf[offset+3],
			      iw_ie_cypher_name, IW_IE_CYPHER_NUM);
 	}
      offset+=4;
    }
  printf("\n");
 
  /* Check if we are done */
  if(ielen < (offset + 2))
    return;

  /* Now, we have authentication suites. */
  cnt = iebuf[offset] | (iebuf[offset + 1] << 8);
  offset += 2;
  printf("Authentication Suites (%d) :", cnt);

  if(ielen < (offset + 4*cnt))
    return;

  for(i = 0; i < cnt; i++)
    {
      if(memcmp(&iebuf[offset], wpa_oui, 3) != 0)
 	{
 	  printf(" Proprietary");
 	}
      else
	{
	  iw_print_value_name(iebuf[offset+3],
			      iw_ie_key_mgmt_name, IW_IE_KEY_MGMT_NUM);
 	}
       offset+=4;
     }
  printf("\n");
 
  /* Check if we are done */
  if(ielen < (offset + 1))
    return;

  /* Otherwise, we have capabilities bytes.
   * For now, we only care about preauth which is in bit position 1 of the
   * first byte.  (But, preauth with WPA version 1 isn't supposed to be 
   * allowed.) 8-) */
  if(iebuf[offset] & 0x01)
    {
      printf("Preauthentication Supported\n");
    }
}
 
/*------------------------------------------------------------------*/
/*
 * Process a generic IE and display the info in human readable form
 * for some of the most interesting ones.
 * For now, we only decode the WPA IEs.
 */
static inline void
iw_print_gen_ie(unsigned char *	buffer,
		int		buflen)
{
  int offset = 0;

  /* Loop on each IE, each IE is minimum 2 bytes */
  while(offset <= (buflen - 2))
    {
      printf("IE: ");
 
      /* Check IE type */
      switch(buffer[offset])
	{
	case 0xdd:	/* WPA1 (and other) */
	case 0x30:	/* WPA2 */
	  iw_print_ie_wpa(buffer + offset, buflen);
	  break;
	default:
	  iw_print_ie_unknown(buffer + offset, buflen);
	}
      /* Skip over this IE to the next one in the list. */
      offset += buffer[offset+1] + 2;
    }
}
#endif	/* WE_ESSENTIAL */

/***************************** SCANNING ****************************
 * Note that we don't use the scanning capability of iwlib (functions
 * iw_process_scan() and iw_scan()). The main reason is that
 * iw_process_scan() return only a subset of the scan data to the caller,
 * for example custom elements and bitrates are ommited. Here, we
 * do the complete job...
 */

// print one element from scan results
static inline void
build_scanning_token(struct stream_descr *	stream,	/* Stream of events */
		     struct iw_event *		event,	/* Extracted token */
		     struct iw_range *	iw_range,	/* Range info */
		     int		has_range,
		     struct iw_results * inresults)
{
  struct 	iw_results results = *inresults;
  struct 	iw_ap ap;
  char		buffer[128];	
  switch(event->cmd)
    {
    case SIOCGIWAP:
      results.num_of_ap = results.num_of_ap + 1;
      char *bssid = malloc(12*sizeof(char));
      iw_sether_ntop(&event->u.ap_addr, bssid);
      strncpy(ap.bssid, bssid, sizeof(bssid));
      strncpy(ap.essid, "", sizeof(""));
      ap.strength = 0;
      results.aps[results.num_of_ap - 1] = ap;
      *inresults = results;
      break;
     case SIOCGIWESSID:
      {	
	char essid[4*IW_ESSID_MAX_SIZE+1];
	memset(essid, '\0', sizeof(essid));
	if((event->u.essid.pointer) && (event->u.essid.length))
	  iw_essid_escape(essid,
			  event->u.essid.pointer, event->u.essid.length);
	if(event->u.essid.flags)
	  {
	    ap = results.aps[results.num_of_ap - 1];
	    strncpy(ap.essid, essid, sizeof(essid));
	    results.aps[results.num_of_ap - 1] = ap;
	  }
	else 
	  { 
	    ap = results.aps[results.num_of_ap - 1];
	    strncpy(ap.essid, "", sizeof(""));
	    results.aps[results.num_of_ap - 1] = ap;
	  }
	*inresults = results;
      }
      break;
    case IWEVQUAL:
       // printf("qual.qual %d\nqual.level %d\nqual.noise %d\n", event->u.qual.qual, event->u.qual.level, event->u.qual.noise);
	ap.strength = event->u.qual.qual;
	results.aps[results.num_of_ap - 1] = ap;
	*inresults = results;
      break;
    case SIOCGIWNWID:
      break; 
    case SIOCGIWFREQ:
      break;
    case SIOCGIWMODE:
      break;
    case SIOCGIWNAME:
      break;
    case SIOCGIWENCODE:
      break;
    case SIOCGIWRATE:
      break;
    case SIOCGIWMODUL:
      break;
    case IWEVGENIE:
      break;
    case IWEVCUSTOM:
      break;
    default:
      printf("(Unknown Wireless Token 0x%04X)\n",
	     event->cmd);
   }	/* switch(event->cmd) */
}

// perform a scan with a device
static int
perform_scan(int		skfd,
		    char *	ifname,
		    char *	args[],		/* Command line args */
		    int		count)		/* Args count */
{
  struct iw_results	results;

  struct iwreq		wrq;
  struct iw_scan_req    scanopt;		/* Options for 'set' */
  unsigned char *	buffer = NULL;		/* Results */
  int			buflen = IW_SCAN_MAX_DATA; /* Min for compat WE<17 */
  struct iw_range	range;
  int			has_range;
  struct timeval	tv;				/* Select timeout */
  int			timeout = 1000000;		/* 1s */


  /* Avoid "Unused parameter" warning */
  args = args; count = count;

  /* Get range stuff */
  has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
  // cycle through all iw_range/iw_range15 struct members and print

  /* Check if the interface could support scanning. */
  if((!has_range) || (range.we_version_compiled < 14))
    {
      fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",
	      ifname);
      return(-1);
    }

  /* Init timeout value -> 25ms between set and first get */
  tv.tv_sec = 0;
  tv.tv_usec = 250;

  /* Clean up set args */
  memset(&scanopt, 0, sizeof(scanopt));

  results.num_of_ap = 0;
 
  //
  /* Initiate Scanning */
  //
  if(iw_set_ext(skfd, ifname, SIOCSIWSCAN, &wrq) < 0)
  {
      if((errno != EPERM))
      {
       fprintf(stderr, "%-8.16s  Interface doesn't support scanning : %s\n\n",
       ifname, strerror(errno));
       return(-1);
      }
     /* fallback to last results (no waiting) */
       tv.tv_usec = 0;
  }
  timeout -= tv.tv_usec;

  /* Forever */
  while(1)
    {
      fd_set		rfds;		/* File descriptors for select */
      int		last_fd;	/* Last fd */
      int		ret;

      /* Guess what ? We must re-generate rfds each time */
      FD_ZERO(&rfds);
      last_fd = -1;

      /* In here, add the rtnetlink fd in the list */

      /* Wait until something happens */
      ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);

      /* Check if there was an error */
      if(ret < 0)
	{
	  if(errno == EAGAIN || errno == EINTR)
	    continue;
	  fprintf(stderr, "Unhandled signal - exiting...\n");
	  return(-1);
	}

      /* Check if there was a timeout */
      if(ret == 0)
	{
	  unsigned char *	newbuf;

	realloc:
	  /* (Re)allocate the buffer - realloc(NULL, len) == malloc(len) */
	  newbuf = realloc(buffer, buflen);
	  if(newbuf == NULL)
	    {
	      if(buffer)
		free(buffer);
	      fprintf(stderr, "%s: Allocation failed\n", __FUNCTION__);
	      return(-1);
	    }
	  buffer = newbuf;

	  /* Try to read the results */
	  wrq.u.data.pointer = buffer;
	  wrq.u.data.flags = 0;
	  wrq.u.data.length = buflen;
	  if(iw_get_ext(skfd, ifname, SIOCGIWSCAN, &wrq) < 0)
	    {
	      /* Check if buffer was too small (WE-17 only) */
	      if((errno == E2BIG) && (range.we_version_compiled > 16)
		 && (buflen < 0xFFFF))
		{
		  /* Some driver may return very large scan results, either
		   * because there are many cells, or because they have many
		   * large elements in cells (like IWEVCUSTOM). Most will
		   * only need the regular sized buffer. We now use a dynamic
		   * allocation of the buffer to satisfy everybody. Of course,
		   * as we don't know in advance the size of the array, we try
		   * various increasing sizes. Jean II */

		  /* Check if the driver gave us any hints. */
		  if(wrq.u.data.length > buflen)
		    buflen = wrq.u.data.length;
		  else
		    buflen *= 2;

		  /* wrq.u.data.length is 16 bits so max size is 65535 */
		  if(buflen > 0xFFFF)
		    buflen = 0xFFFF;

		  /* Try again */
		  goto realloc;
		}

	      /* Check if results not available yet */
	      if(errno == EAGAIN)
		{
		  /* Restart timer for only 100ms*/
		  tv.tv_sec = 0;
		  tv.tv_usec = 100000;
		  timeout -= tv.tv_usec;
		  if(timeout > 0)
		    continue;	/* Try again later */
		}

	      /* Bad error */
	      free(buffer);
	      fprintf(stderr, "%-8.16s  Failed to read scan data : %s\n\n",
		      ifname, strerror(errno));
	      return(-2);
	    }
	  else
	    /* We have the results, go to process them */
	    break;
	}

      /* In here, check if event and event type
       * if scan event, read results. All errors bad & no reset timeout */
    }

  if(wrq.u.data.length)
    {
      struct iw_event		iwe;
      struct stream_descr	stream;
      int			ret;

      iw_init_event_stream(&stream, (char *) buffer, wrq.u.data.length);
      do
	{

	  /* Extract an event and print it */
	  ret = iw_extract_event_stream(&stream, &iwe,
					range.we_version_compiled);
	  if(ret > 0) {
	    build_scanning_token(&stream, &iwe,
				 &range, has_range, &results);
	  }
	}
      while(ret > 0 && (results.num_of_ap < MAX_NUM_RESULTS));

      struct iw_trimmed_results { 
         uint8_t	num_of_ap;
	 struct iw_ap   aps[results.num_of_ap];
      } trimmed_results;

      trimmed_results.num_of_ap = results.num_of_ap;

      printf("%d RESULTS\n", results.num_of_ap);
      char *iw2log = "/tmp/iw2log";
      mkfifo(iw2log, 0666);
      int fd;
      for (int i = 0; i < results.num_of_ap; i++) {
	  trimmed_results.aps[i] = results.aps[i];
          printf("==== %d\n%s\n", i, trimmed_results.aps[i].bssid);
          printf("%s\n", trimmed_results.aps[i].essid);
          printf("%d\n", trimmed_results.aps[i].strength);
      }
      fd = open(iw2log, O_WRONLY);
      printf("writing to FIFO\n");
      write(fd, &trimmed_results, sizeof(trimmed_results));
      write(fd, trimmed_results.aps, sizeof(struct iw_ap)*trimmed_results.num_of_ap + 1);
      printf("done writing, closing FIFO\n");
      close(fd);
      printf("closed FIFO\n");
  }
  else
    printf("%-8.16s  No scan results\n\n", ifname);

  free(buffer);
  return(0);
}


/******************************* MAIN ********************************/

/*------------------------------------------------------------------*/
/*
 * The main !
 */
int
main(int	argc,
     char **	argv)
{
  int skfd;			/* generic raw socket desc.	*/
  char *dev;			/* device name			*/
  char *cmd;			/* command			*/
  char **args;			/* Command arguments */
  int count;			/* Number of arguments */
  int goterr = 0;

  if(argc == 2)
    {
      dev = argv[1];
      args = NULL;
      count = 0;
    }
  else
    {
      dev = argv[1];
      args = argv + 2;
      count = argc - 2;
    }

  /* Create a channel to the NET kernel. */
  if((skfd = iw_sockets_open()) < 0)
    {
      perror("socket");
      return -1;
    }

  /* do the actual work */
  if (dev)
    goterr = perform_scan(skfd, dev, args, count);
  else
    iw_enum_devices(skfd, perform_scan, args, count);

  /* Close the socket. */
  iw_sockets_close(skfd);

  return goterr;
}
