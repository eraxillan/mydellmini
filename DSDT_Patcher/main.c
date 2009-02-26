#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define VERSION "1.0.1e"

void patchVersion ();

// Variables
int debug=0, forceBuild=0, currLine=0, open=0, otherDSDTFileGiven=0, writeFixedHPET=0;
char s[128]; // Line Buffer

// Paths to files
char origDSDTPath[50] = "./dsdt.dsl", patchedDSDTPath[] ="./dsdt_fixed.txt"; 

// THis is the HPET device we write if no HPET device is found
char fixedHPET[]	= "		Device (HPET)\n                {\n                    Name (_HID, EisaId (\"PNP0103\"))\n                    Name (ATT3, ResourceTemplate ()\n                    {\n                        IRQNoFlags ()\n                            {0}\n                        IRQNoFlags ()\n                            {8}\n                        Memory32Fixed (ReadWrite,\n                            0xFED00000,         // Address Base\n                            0x00000400,         // Address Length\n                            )\n                    })\n                    Name (ATT4, ResourceTemplate ()\n                    {\n                    })\n                    Method (_STA, 0, NotSerialized)\n                    {\n                        Return (0x0F)\n                    }\n                    Method (_CRS, 0, NotSerialized)\n                    {\n                        Return (ATT3)\n                    }\n                }\n";

// These are the IRQ's that are required in HPET
char HPETIRQ[]		= "                        IRQNoFlags ()\n                            {0}\n                        IRQNoFlags ()\n                            {8}\n";

// File pointers
FILE *origDSDT, *patchedDSDT;

// Findigs
int HPETDeviceFound=0;
int RTCDeviceFound=0;
int RTCIRQFound=0;

// Device Strings
char RTCDevice[] = "Device (RTC";				// RTC device
char RTCDevice2[] = "PNP0B00";					// RTC device
char HPETDevice[] = "Device (HPET";				// HPET device
char HPETDevice2[] = "PNP0103";					// HPET device

char IRQ[] = "IRQNoFlags ()";					// needed for RTC fix
char RESOURCETEMP[] = "ResourceTemplate ()";	// search for this entry when we dont find "human" device name like HPET but PNP0103
char PROCESSOR[] = "Processor (";				// processor string, we need this for searching for cpu aliases

// various issues
char issue1[] = ", \"*";						// devices must not begin with *   saw it on some dell
char issue2[] = "Alias (";						// CPU aliases
char issue3[] = "Store (Local0, Local0)"; char fix3[] = "Store (\"Local0\", Local0)";		// Local0 issue
char issue4[] = "_T_";
char issue5[] = "Acquire (MUTE, 0x????)";
char fix5[] = "Acquire (MUTE, 0xFFFF)\n";

void flagCheck(int argc, const char *argv[]) {
	if (argc>1) {
		int i=1;
		for (i;i<argc;i++) {
			if (!strcmp(argv[i], "-d")) debug=1;			// checks for debug flag
			else if (!strcmp(argv[i], "-f")) forceBuild=1;	// checks for force build flag
			else if (!strcmp(argv[i], "-newHPET")) writeFixedHPET=1;
			else {											// checks for an other dsdt.dsl to patch
				sprintf(origDSDTPath, "%s", argv[i]);
				otherDSDTFileGiven=1;
			}
		}
	}
}	// checks for flags given to the tool

void cwd(const char *argv[]) {
	char dir[strlen(argv[0])];
	int i=strlen(argv[0]);
	sprintf(dir, "%s", argv[0]);
	
	for (i=strlen(dir);i>=0;i--) { 
		if (dir[i] == '/') {
			dir[i] = '\0';
			break;
		}
	}
	chdir(dir);
}		// change the current working directory

int main (int argc, const char * argv[]) {
    
	// Checking for flags
	flagCheck(argc, argv);
	cwd(argv);
	
	system("clear");
	printf("DSDT Patcher %s -- report Bugs to superfassl@gmail.com\n",VERSION);
	printf("enclose \"DSDT Patcher/Debug/USER.tar\"\n\n\n");
	printf("Press any key to continue...");
	printf("\n");
	system("read");
	if (!otherDSDTFileGiven) {								// if no other file is set in the args, get the dsdt.dsl with getdsdt tool and decompile it
		printf("\n\nGetting the DSDT through ioreg...\n");
		system("./Tools/getDSDT.sh");
		printf("\n\n\nDecompiling the DSDT...\n");
		system("./Tools/iasl -d ./dsdt.dat");
		system("clear");
	}
	printf("\n\n\nDone, now start the Patching!\n\n");
	
	// Patching the RTC here
	if(!patchRTC()) goto errorRTC;						// Patch the RTC and save to rtc_fixed.txt
	// Patching the HPET here
	if(!patchHPET()) goto errorHPET;					// Patch the HPET and save to hpet_fixed.txt
	patchVersion();						
	// Patching various issues
	if(!patchVarious()) goto errorVarious;				// Patch various issues and save to dsdt_fixed.txt
	
	printf("\n\n\nWe are done patching, press any key to try to compile the fixed DSDT\n\n");
	
	scanf(":");
	if(forceBuild)										// when -f flag is set we force the build
		system("./Tools/iasl -ta -f ./dsdt_fixed.txt");
	else												// otherwise we compile it without forcing
		system("./Tools/iasl -ta ./dsdt_fixed.txt");
		
	printf("\n\n\nCompiling done, if it worked, you have now a patched DSDT in dsdt.aml\nIf the compiling went wrong, you could force to build it with ./DSDT\\ Patcher -f (try this DSDT at your own risk)\n\n\n");
	
	// Clean up & make a tar for debug
	system("rm dsdt_fixed.hex && rm dsdt.dat");			
	system("mkdir ./Debug");
	system("mv ./rtc_fixed.txt ./Debug && mv ./hpet_fixed.txt ./Debug && mv ./dsdt_fixed.txt ./Debug && mv ./dsdt.dsl ./Debug");
	system("tar -czf ./Debug/$USER.tar ./Debug/*");
	return 0;
	
	// RTC patching failed
	errorRTC:
	printf("There were errors pacthing the RTC\n\n");
	return 0;
	
	// HPET patching failed
	errorHPET:
	printf("There were errors pacthing the HPET\n\n");
	return 0;
	
	// fixing other issues failes
	errorVarious:
	printf("There were errors pacthing various Issues\n\n");
	return 0;
}

void closeFiles() {
	fclose(origDSDT);
	fclose(patchedDSDT);
}										// close the files
int cmpStr(char *searchString, char *searchTerm) {
	int i;
	for (i = 0 ; i <= strlen(searchString); ++i )
	{
		if ( strncmp( &searchString[ i ], searchTerm, strlen(searchTerm) ) == 0 ) return 1;
	}
	return 0;
}		// compares 2 strings
int cmpStrWild(char *searchString, char *searchTerm) {
  int i, j;
  for (i = 0 ; i +strlen(searchTerm) < strlen(searchString); ++i )
	{
	  for (j = 0; j<strlen(searchTerm);j++)
	    if (searchString[i+j]!=searchTerm[j] && searchTerm[j]!='?')
	      break;
	  if ( j==strlen(searchTerm) ) return 1;
	}
	return 0;
}		// compares 2 strings
int cmpStr2(char *searchString, char *searchTerm, int i) {
	int x=0;
	for (x ; x < strlen(searchTerm); ++x )
	{
		if (searchString[i+x]!=searchTerm[x]) return 0;
	}
	return 1;
}
void replaceAlias(char *string, char *string2, int i) {
	int x=0;
	for(;x<strlen(string2);x++) {
		string[i+x]=string2[x];
	}
}

int foundRTCDevice (char *s) {
	if (cmpStr(s, RTCDevice)) {								// compare to "Device (RTC"
		RTCDeviceFound=1;
		printf("RTC Device found : %s",s);
		return 1;
	} else if (cmpStr(s, RTCDevice2) && !RTCDeviceFound) {	// compare to "PNP0B00"
		RTCDeviceFound=1;
		printf("RTC Device found : %s",s);
		open++;								// if we found this it means there is already an open { so we need to increment it, otherwise the routine wouldnt find the HPET device end
		return 1;
	}
	return 0;
}							// found an RTC device?
int foundHPETDevice (char *s) {
	if (cmpStr(s, HPETDevice)) {							// compare to "Device (HPET"
		HPETDeviceFound=1;
		printf("HPET Device found : %s",s);
		return 1;
	} else if (cmpStr(s, HPETDevice2) && !HPETDeviceFound) {	// compare to "PNP0103"
		HPETDeviceFound=1;
		printf("HPET Device found : %s",s);
		open++;								// if we found this it means there is already an open { so we need to increment it, otherwise the routine wouldnt find the RTC device end
		return 1;
	}
	return 0;
}							// found an HPET device?

int patchRTC() {
	if ((origDSDT=fopen(origDSDTPath,"r"))==NULL) {
		printf("Could not open file %s\n\n",origDSDTPath);
		return 0;
	}							// Open Files
	if ((patchedDSDT=fopen("./rtc_fixed.txt","w"))==NULL) {
		printf("Could not create file ./rtc_fixed.txt\n\n");
		return 0;
	}					// Open Files
		
	open = 0;				// count of open {
	int start = 0;
	
	printf("Patching RTC...\n\n");
	currLine=0;
	
	// Patching the RTC -> removing IRQ
	while(!feof(origDSDT)) {
		if(fgets(s,126,origDSDT)) {
			currLine++;	
			// Write as is until we reach the RTC Device
			if (foundRTCDevice(s)) {
				// We reached to RTC Device, search for IRQ now
				fprintf(patchedDSDT,s);		// Write the Device line
			
				start++;
			
				while(start) {			// While we didnt reach the end of the RTC Device
					fgets(s,126,origDSDT);	// Get a new Line
					if (cmpStr(s,"{")) open++;		// If its a { we increment open
					if (cmpStr(s,"}")) open--;	// If its a } we decrement open
					if(!open) {				// If there are no more open { we reached the Device RTC end -> break
						printf("No IRQ found in RTC Device, should be fine\n");
						break;
					}
					if (cmpStr(s,IRQ)) {			// Search for IRQ
						printf("Found IRQ in RTC Device, removing it\n");	// We found one
						printf("%s",s);
						fgets(s,126,origDSDT); fgets(s,126,origDSDT);		// Skip the IRQ
						printf("RTC patched\n\n");
						break;
						open=0;												// We are done here and say there are no more open {
					}
					if(open!=0) fprintf(patchedDSDT,s);	// While we havent reached the Device RTC end we write the lines to the patched dsdt
				}
			}
			fprintf(patchedDSDT,s);			// Write the rest as it is to the patched DSDT
		}
	}
	closeFiles();

	return 1;
}										// Patch the RTC here
int patchHPET() {
	if ((origDSDT=fopen("./rtc_fixed.txt","r"))==NULL) {
		printf("Could not open file ./rtc_fixed.txt\n\n");
		return 0;
	}				// Open Files
	if ((patchedDSDT=fopen("./hpet_fixed.txt","w"))==NULL) {
		printf("Could not create file ./hpet_fixed.txt\n\n");
		return 0;
	}
	open = 0;				// count of open {
	int start = 0;
	
	printf("Patching HPET...\n\n");
	
	if (writeFixedHPET) {
		if(!HPETDeviceFound) {												// If there is no HPET Device in your DSDT already
			printf("HPET Device will be overwritten...\n");
		
			open=0;						// count of open {
			int written=0;				// stores if the HPET is already written
			HPETDeviceFound=0;
			while(!feof(origDSDT)) {
				if(fgets(s,126,origDSDT)) {
					foundHPETDevice(s);
				}
			}
			if(!HPETDeviceFound) goto nextone;
			fseek(origDSDT,0,SEEK_SET);
			// We will search for the RTC device and add a HPET device after that
			while(!feof(origDSDT)) {
				if(fgets(s,126,origDSDT)) {
					if(foundHPETDevice(s)) {				// RTC Device found
						while(1) {
							fgets(s,126,origDSDT);
							if (cmpStr(s,"{")) open++;
							if (cmpStr(s,"}")) open--;
							if (!open) break;
							printf("%i: %s\n",open,s);
						}
						if(!written) {						// we reached the end of rtc device, write the hpet here
							fprintf(patchedDSDT,fixedHPET);
							printf("New HPET written\n\n");
							written=1;
							fgets(s,126,origDSDT);		// get next line
						}
					}
					fprintf(patchedDSDT,s);		// write that line
				}
			}
			closeFiles();					// close the files
			return 1;
		}
	}
	
	
	// Patching the HPET -> adding IRQ's
	while(!feof(origDSDT)) {
		if(fgets(s,126,origDSDT)) {
		
			// Write as is until we reach the HPET
			if (foundHPETDevice(s)) {
				// We reached to HPET Device, search for "ResourceTemplate ()" now
				fprintf(patchedDSDT,s);		// Write the Device line
			
				start++;
			
				while(start) {			// While we didnt reach the end of the HPET Device
					fgets(s,126,origDSDT);	// Get a new Line
					if (cmpStr(s,"{")) open++;		// If its a { we increment open
					else if (cmpStr(s,"}")) open--;	// If its a } we decrement open
					if(!open) break;				// If there are no more open { we reached the Device HPET end -> break
					if (cmpStr(s,RESOURCETEMP)) {			// Search for "ResourceTemplate ()"
						fprintf(patchedDSDT,s);						// Write the "ResourceTemplate ()"
						fgets(s,126,origDSDT);						// Get the {
						fprintf(patchedDSDT,s);						// Write the {
						fprintf(patchedDSDT,HPETIRQ);				// Write the IRQ's to HPET Device
						fgets(s,126,origDSDT);						// We get the next line
						while(cmpStr(s,IRQ)) {						// If there were already IRQ's skip that ones
							fgets(s,126,origDSDT); fgets(s,126,origDSDT);
						}
						fprintf(patchedDSDT,s);
						printf("IRQ's written to HPET\n");
						printf("HPET patched\n");
						open=0;												// We are done here and say there are no more open {
					}

					if(open!=0) fprintf(patchedDSDT,s);	// While we havent reached the Device HPET end we write the lines to the patched dsdt

				}
			
			}
			fprintf(patchedDSDT,s);			// Write the rest to the patched DSDT
		}
	}
	nextone:
	closeFiles();
	
	if(!HPETDeviceFound) {												// If there is no HPET Device in your DSDT already
		printf("No HPET Device found, adding one\n");
		
		if ((origDSDT=fopen("./rtc_fixed.txt","r"))==NULL) {
			printf("Could not open file ./rtc_fixed.txt\n\n");
			return 0;
		}		// Open File
		if ((patchedDSDT=fopen("./hpet_fixed.txt","w"))==NULL) {
			printf("Could not create file ./hpet_fixed.txt\n\n");
			return 0;
		}		// Open File
		
		open=0;						// count of open {
		int written=0;				// stores if the HPET is already written
		RTCDeviceFound=0;
		
		// We will search for the RTC device and add a HPET device after that
		while(!feof(origDSDT)) {
			if(fgets(s,126,origDSDT)) {
				if(foundRTCDevice(s)) {				// RTC Device found
					fprintf(patchedDSDT,s);			// wrtie the Device (RTC
					while(1) {
						fgets(s,126,origDSDT);
						if (cmpStr(s,"{")) open++;
						if(cmpStr(s,"}")) open--;
						fprintf(patchedDSDT,s);
						if (!open) break;
					
					}
					if(!written) {						// we reached the end of rtc device, write the hpet here
						fprintf(patchedDSDT,fixedHPET);
						printf("New HPET written\n\n");
						written=1;
						fgets(s,126,origDSDT);		// get next line
					}
				}
				fprintf(patchedDSDT,s);		// write that line
			}
		}
		closeFiles();					// close the files
	}
	return 1;
}
void patchVersion () {
  int v;
  char *inbuf, *outbuf;
  FILE *f;
  char *replfrom, *replto, *inptr,*outptr;
  int flen;
  printf ("Which OS to emulate? [0=Darwin,1=WinXP, 2=WinVista]\n");
  scanf ("%d", &v);
  switch (v)
    {
    case 1:
      replfrom="_OSI (\"Windows 2001\")";
      replto="LOr (_OSI (\"Darwin\"), _OSI (\"Windows 2001\"))";
      break;
    case 2:
      replfrom="_OSI (\"Windows 2006\")";
      replto="LOr (_OSI (\"Darwin\"), _OSI (\"Windows 2006\"))";
      break;
    default:
      return;
    }
  f=fopen ("./hpet_fixed.txt", "rb");
  fseek (f, 0, SEEK_END);
  flen=ftell (f);
  fseek (f, 0, SEEK_SET);
  inbuf=malloc (flen);
  fread (inbuf, flen,1, f);
  fclose (f);
  outbuf=malloc (2*flen);
  for (inptr=inbuf,outptr=outbuf;inptr<inbuf+flen-strlen (replfrom);)
    {
      if (memcmp (inptr,replfrom,strlen (replfrom)))
	{
	  *outptr=*inptr;
	  outptr++;
	  inptr++;
	  continue;
	}
      memcpy (outptr, replto, strlen (replto));
      outptr+=strlen (replto);
      inptr+=strlen (replfrom);
    }
  memcpy (outptr, inptr, flen-(inptr-inbuf));
  outptr+=flen-(inptr-inbuf);
  f=fopen ("./hpet_fixed.txt", "wb");
  fwrite (outbuf, outptr-outbuf,1,f);
  fclose (f);
  
}										// Patch the HPET here
int patchVarious() {
	
	char *ALIAS;
	char *ALIAS2;
	char *buff;
	
	int firstfreet=0; // First unused prefix of form T%d_
	char freetprefix[40];
	
	starthere:
	
	if ((origDSDT=fopen("./hpet_fixed.txt","r"))==NULL) {
		printf("Could not open file ./hpet_fixed.txt\n\n");
		return 0;
	}			// open files
	if ((patchedDSDT=fopen("./dsdt_fixed.txt","w"))==NULL) {
		printf("Could not create file ./dsdt_fixed.txt\n\n");
		return 0;
	}
	
	printf("Fixing various Issues...\n\n");
	
	/*Finding first unused prefix of form T%d_. Ugly, inefficient but gets job done*/
	for (firstfreet=0;;firstfreet++)
	{
		int isfree=1;
		sprintf(freetprefix,"T%d_",firstfreet);
		while(!feof(origDSDT))
			if(fgets(s,126,origDSDT) && cmpStr(s,freetprefix))
			{
				isfree=0;
				break;
			}
		fseek(origDSDT,0,SEEK_SET);
		if (isfree)
			break;
	}
	
	while(!feof(origDSDT)) {
		if(fgets(s,126,origDSDT)) {
		
			if(cmpStr(s,issue1)) {				// Found an issue, saw this until yet just for dell dsdt, we skip the * in device name
				printf("Found an issue\n\n");
				int i=0;
				char buff[100];
				int add=0;
			
				for(;i<strlen(s);i++) {
					if(s[i]=='*') {
						printf("Found");
						add=1;
					}
					strcpy(&buff[i],&s[i+add]); //buff[i]=s[i+add];
				}
				printf("%s",s);
				printf("Fixed %s\n\n",buff);
				fprintf(patchedDSDT,buff);		// write the modified line
				goto dontwrite;			
			}					// Device Name fix
			
					
			if(cmpStr(s,PROCESSOR)) {				// Here is the CPU Aliases fix, we just skip the alias lines
				fprintf(patchedDSDT,s);
				if(fgets(s,126,origDSDT)) {
					if(cmpStr(s,issue2)) {
						printf("Found an issue\n");
						printf("Found   %s",s);
						printf("Skipped %s\n",s);
						int i=0;
						int b=0;
					
						ALIAS=malloc(strlen(s)+2);
						ALIAS2=malloc(strlen(s)+2);
						buff=malloc(strlen(s)+2);
					
						strcpy(buff,s);
					
						for(;i<=strlen(s);i++) {
						if(s[i]=='(') {
							i++;
							for(;i<=strlen(s);i++) {
								if(s[i]!=',') {
									ALIAS[b] = s[i];
									b++;
								} else goto next;
							}
						}
					}
						next:
						ALIAS[b+1]='\0';
						i+=2;
						b=0;
						for(;i<=strlen(s);i++) {
						if(s[i]!=')') {
							ALIAS2[b] = s[i];
							b++;
						 } else break;
					}
						next2:
						ALIAS2[b+1]='\0';
					
						while(!feof(origDSDT)) {
							if(fgets(s,126,origDSDT)) {
								if(cmpStr(s, ALIAS2)) {
									buff=malloc(strlen(s)+2);
									strcpy(buff,s);
									i=0;
									while(!cmpStr2(s,ALIAS2,i)) i++;
									printf("Found    %s",buff);
									replaceAlias(s,ALIAS,i);
									printf("Replaced %s",s);
								}
								fprintf(patchedDSDT,s);
							}
						}
						closeFiles();
						system("cp ./dsdt_fixed.txt ./hpet_fixed.txt");
						origDSDT=fopen("./hpet_fixed.txt","r");
						patchedDSDT=fopen("./dsdt_fixed.txt","w");
					}
				}
			}				// CPU Aliases Fix
			
			if(cmpStr(s,issue3)) {						// "Method local variable is not initialized (Local0)" fix
				fprintf(patchedDSDT,fix3);
				printf("Found an issue\n");
				printf("Found   %s",s);
				printf("Fixed   %s\n", fix3);	
				goto dontwrite;
			}
			if(cmpStr(s,issue4)) {						// "_T_" fix
				char *tmp, *tmpptr;
				int i;
				printf("Found an issue\n");
				printf("Found   %s",s);
				tmpptr=tmp=malloc(strlen(s)*5+10);
				for (i = 0 ; i <= strlen(s); )
				{
					if ( strncmp(s+i, issue4, strlen(issue4) ) == 0 )
					{
						strcpy(tmpptr,freetprefix);
						tmpptr+=strlen(freetprefix);
						i+=strlen(issue4);
					}
					else 
					{
						*tmpptr=s[i];
						tmpptr++;
						i++;
					}

				}
				*tmpptr=0;
				fprintf(patchedDSDT,tmp);
				printf("Fixed   %s\n\n", tmp);
				free(tmp);
				goto dontwrite;
			}
		
			if(cmpStrWild(s,issue5) && !cmpStr(s,fix5)) {						// "Mute fix
				fprintf(patchedDSDT,fix5);
				printf("Found an issue\n");
				printf("Found   %s",s);
				printf("Fixed   %s\n", fix5);	
				goto dontwrite;
			}
		
			fprintf(patchedDSDT,s);				// Write the line
			dontwrite:
			printf("");
			//printf("%s",s);
		}
	}
	closeFiles();						// Close files
	
	printf("Done\n\n");

	return 1;
}									// Patch various issues here
