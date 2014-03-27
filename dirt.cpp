/**********************************************
 * Disk Image Recovery Tool (DIRT)
 * -------------------------------------------
 * Created By        Marcus Kelly
 *    Contact        nexuslite@gmail.com
 * Created On        Feb 23, 2014
 * Updated On        Mar 27, 2014
 * -------------------------------------------
 * Copyright 2014 Marcus Kelly
 **********************************************/
#include <iostream>
#include <fstream>
#include <cerrno>
#include <stdexcept>
#include <cstring>
#include <vector>   /* vector */
#include <stdlib.h> /* atoi */
#include <sys/time.h> /* SetTimer & CheckTimer */
#include <stdio.h>

#include <sstream>  /* uchar2hex */
#include <iomanip>  /* uchar2hex */

//#include <stdint.h> /* int64_t */
//#include <cstdint>

//#include <iostream> /* str.copy */
//#include <string> /* str.copy */


using namespace std;

#define BUF_SIZE 512
#define MAX_SEEK 2147483648

string VersionHigh = "0";
string VersionLow = "2a";
int raidLevel = 1;
string fileSystem = "ext4";
int raidStartBlock = 0;
int raidBlockSize = 65536;
int bufferSize = BUF_SIZE;
string drives[10];
vector<vector <char> > buffer;
int currentDrive = 0;
int maxDrives = 10;
int dumpIndex = 0;
int searchIndex = 0;
int searchIndexFlag = 0;
unsigned int dumpRaidSector = 0;
int ignoredDrive = -1;
streampos dsize = 0;
int inode;
int BlockAdjust=0;
unsigned int CurrentDirectory = 0;
string filename;

void help() {
  cout << "---------------------------------------\n";
  cout << "  a=/dev/sdb           Add Drive\n";
  cout << "  bs                   Block Size Selection Menu\n";
  cout << "  d[=block]            Dump Drive Block as ASCII (Next if no number given)\n";
  //cout << "  f=[fs]               Change File System [ext4, fat, ntfs]
  cout << "  h                    Help\n";
  cout << "  inode[=inode]        Display Inode (Next if no number given)\n";
  cout << "  l                    List Drives\n";
  //cout << "  o                    Order Drives\n";
  cout << "  ba=[block]           Raid Block Adjust, Added to the Current Block for Proper Rotation\n";
  cout << "  rbs                  Raid Block Size Selection Menu\n";
  cout << "  rd[=block]           Dump Raid Block as ASCII (Next if no numer given)\n";
  cout << "  rl=[level]           Set Raid Level [0, 1, 2, 3, 4, 5, 6]\n";
  cout << "  rsb=[block]          Raid Start in Drive Block (dependent on Block Size)\n";
  cout << "  rs=[string]          Raid Search\n";
  cout << "  ru[=block]           Dump Raid Block as HEX (Next if no number given)\n";
  cout << "  sd=[sector]          Set Directory\n";
  cout << "  dir | ls             List Directory\n";
  cout << "  cd=[name]            Change Directory\n";
//  cout << "  fd=[name]            File Dump (Saves file in current system directory)\n";
//  cout << "  save                 Save Configuration (Saves in current system directory)\n";
//  cout << "  load                 Load Configuration (Loads in current system directory)\n";
  cout << "  pull=[inode]         Pull from drive and save in current directory as inode #\n";
  cout << "  stat                 Display current configuration\n";
  cout << "  s=[string]           Search\n";
  cout << "  ss=[block]           Start search from block\n";
  cout << "  u[=block]            Dump Drive Block as HEX (Next if no number given)\n";
  cout << "  q                    Quit\n";
  cout << "---------------------------------------\n";

}

void stat() {
  cout << "---------------------------------------\n";
  cout << "Block Size: " << bufferSize << "\n";
  cout << "Raid Level: " << raidLevel << "\n";
  cout << "Raid Start Block: " << raidStartBlock << "\n";
  cout << "Raid Block Size: " << raidBlockSize << "\n";
  cout << "Raid Block Adjust: " << BlockAdjust << "\n";
  cout << "File System: " << fileSystem << "\n";
  cout << "Current Directory: " << CurrentDirectory << "\n";
  cout << "---------------------------------------\n";
}

string uchar2hex(unsigned char inchar) {
  ostringstream oss (ostringstream::out);
  oss << setw(2) << setfill('0') << hex << (int)(inchar);
  return oss.str();
}

int setTimer(struct timeval &tv, time_t sec) {
    gettimeofday(&tv,NULL);
    tv.tv_sec+=sec;
 
    return 1;
}
 
int checkTimer(struct timeval &tv, time_t sec) {
    struct timeval ctv;
    gettimeofday(&ctv,NULL);
 
    if( (ctv.tv_sec > tv.tv_sec) )
    {
        gettimeofday(&tv,NULL);
        tv.tv_sec+=sec;
        return 1;
    }
    else
        return 0;
}

void setBlockSize() {
      string opt;
      cout << "1:  512\n";
      cout << "2: 1024\n";
      cout << "3: 2048\n";
      cout << "4: 4096\n";
      cout << "5: 8192\n\n";
      cout << ":";
      cin >> opt;
      if (opt.compare(0, 1, "1") == 0) {
        bufferSize = 512;
      }
      if (opt.compare(0, 1, "2") == 0) {
        bufferSize = 1024;
      }
      if (opt.compare(0, 1, "3") == 0) {
        bufferSize = 2048;
      }
      if (opt.compare(0, 1, "4") == 0) {
        bufferSize = 4096;
      }
      if (opt.compare(0, 1, "5") == 0) {
        bufferSize = 8192;
      }
      for(int i = 0; i < maxDrives; i++) {
        buffer[i].resize(bufferSize);
      }
}

void setRaidBlockSize() {
      string opt;
      cout << "1:  16K\n";
      cout << "2:  32K\n";
      cout << "3:  64K\n";
      cout << "4: 128K\n";
      cout << "5: 256K\n";
      cout << "6: 512K\n";
      cout << ":";
      cin >> opt;
      if (opt.compare(0, 1, "1") == 0) {
        raidBlockSize = 16384;
      }
      if (opt.compare(0, 1, "2") == 0) {
        raidBlockSize = 32768;
      }
      if (opt.compare(0, 1, "3") == 0) {
        raidBlockSize = 65536;
      }
      if (opt.compare(0, 1, "4") == 0) {
        raidBlockSize = 131072;
      }
      if (opt.compare(0, 1, "5") == 0) {
        raidBlockSize = 262144;
      }
      if (opt.compare(0, 1, "6") == 0) {
        raidBlockSize = 524288;
      }
}

void zeroIgnoredDrive() {
      /* Set and zero ignored drive */
      for(int i = 0; i < currentDrive; i++) {
        if (drives[i].compare("") == 0) {
          ignoredDrive = i;
          for (int c = 0; c < bufferSize; c++) {
            buffer[ignoredDrive][c] = 0;
          }
          continue;
        }
      }
}

int readDrives(int Index) {
      /* Read content from drives */
      for(int i = 0; i < currentDrive; i++) {
        if (ignoredDrive == i) {
          continue;
        }
        string diskError = string() + drives[i] + ": ";
        ifstream disk(drives[i].c_str(), ios_base::binary);
        if(!disk)
          throw(runtime_error(diskError + strerror(errno)));

        /* Determin size of disk */ /* disk size should be it's own function and put disk sizes into an array */
        dsize = disk.tellg();
        disk.seekg( 0, ios::end );
        dsize = disk.tellg() - dsize;

        if ((bufferSize*Index+bufferSize) >= (dsize)) { /* this needs fixed */
          return ( 1 );
        }

        int IndexCopy = Index;
        disk.seekg(0, ios::beg);
        while(IndexCopy >= MAX_SEEK/bufferSize) {
            disk.seekg(MAX_SEEK, ios::cur);
            IndexCopy -= MAX_SEEK/bufferSize;
        }
        disk.seekg(IndexCopy*bufferSize, ios::cur);

        if(!disk) {
          cout << Index << "\n";
          throw(runtime_error(diskError + std::strerror(errno)));
	}
        disk.read(&buffer[i][0], bufferSize);
        if(!disk) {
          cout << Index << "\n";
          throw(runtime_error(diskError + std::strerror(errno)));
        }

	disk.close();
      }
  return ( 0 );
}

void createIgnoredDrive() {
  if (ignoredDrive >= 0) {
    for (int j = 0; j < currentDrive; j++) {
      if (ignoredDrive == j) {
        continue;
      }
      for(int i = 0; i < bufferSize; i++) {
        buffer[ignoredDrive][i] ^= buffer[j][i];
      }
    }
  }
}

int ReadRAID(unsigned int sector) {
	int TotalDisks, DataDisks, DataDrive;
	unsigned int BlockNumber, DriveBlock, SectorsPerBlock;
	if (raidLevel == 5) {
        	TotalDisks = currentDrive;
        	DataDisks = currentDrive-1;
		SectorsPerBlock = raidBlockSize/bufferSize;
		BlockNumber = (sector/SectorsPerBlock)+BlockAdjust;
        	DriveBlock = sector%(SectorsPerBlock)+(BlockNumber/(DataDisks)*SectorsPerBlock)+raidStartBlock;
		DataDrive = BlockNumber%(TotalDisks);
	}
	else {
		DriveBlock = CurrentDirectory;
		DataDrive = 0;
	}

        zeroIgnoredDrive();
        readDrives(DriveBlock);
        createIgnoredDrive();
	return DataDrive;
}

int main() {
  int run = 1;

  /* Resize the drive buffers */
  buffer.resize(maxDrives);
  for(int i = 0; i < maxDrives; i++) {
    buffer[i].resize(bufferSize);
  }

  /* Print program name and copyright info */
  cout << "Disk Image Recovery Tool (DIRT) - Version " << VersionHigh << "." << VersionLow << "\n";
  cout << "   Copyright 2014 Marcus Kelly\n";

  cout << "\nType h for help\n";

  /* Main loop */
  while(run) {
    string cmd;
    cout << "\n- ";
    cin >> cmd;

    /* Add drive command */
    if (cmd.compare(0, 1, "a") == 0) {
      if (cmd.length() < 2) {
         cout << "Invalid input for command a\n";
         continue;
      }
      if (currentDrive >= maxDrives) {
        cout << "No more drives may be added\n";
        continue;
      }
      cout << "Adding " << cmd.substr(2, cmd.length()-1) << "\n";
      drives[currentDrive] = cmd.substr(2, cmd.length()-1);
      currentDrive++;
    }
    else if (cmd.compare(0, 2, "bs") == 0) {
        setBlockSize();
    }
    else if (cmd.compare(0, 4, "stat") == 0) {
        stat();
    }
    else if (cmd.compare(0, 3, "rbs") == 0) {
        setRaidBlockSize();
    }
    else if (cmd.compare(0, 3, "rsb") == 0) {
      if (cmd.length() < 4) {
         cout << "Invalid input for command rsb\n";
         continue;
      }
      raidStartBlock = atoi(cmd.substr(4, cmd.length()-1).c_str());
    }
    else if (cmd.compare(0, 2, "sd") == 0) {
      if (cmd.length() < 3) {
         cout << "Invalid input for command sd\n";
         continue;
      }
      CurrentDirectory = atoi(cmd.substr(3, cmd.length()-1).c_str());
    }
    else if (cmd.compare(0, 2, "cd") == 0) {

        if (cmd.length() < 3) {
           cout << "Invalid input for command cd\n";
           continue;
        }

        string Directory = cmd.substr(3, cmd.length()-1);
	int NameLength = cmd.length()-3;

	int  DataDrive;
	DataDrive = ReadRAID(CurrentDirectory);
	int CDPos = 1;

	int pos = 0;
	unsigned int name_len, rec_len;
	int i, file_type;
	int BoundryCross = 0;
	int BoundryCrossPos = 0;
	int BoundryIndex = 0;
	char BoundryData[8];

	do {

		if (BoundryCross == 1) {
			rec_len = ((unsigned int) (((unsigned char) BoundryData[5]) << 8) + ((unsigned char) BoundryData[4]));
			name_len = ((unsigned char) BoundryData[6]);
			file_type = ((unsigned char) BoundryData[7]);
			inode = ((unsigned int) (((unsigned char) BoundryData[3]) << 24) + (((unsigned char) BoundryData[2]) << 16) + (((unsigned char) BoundryData[1]) << 8) + ((unsigned char) BoundryData[0]));
			BoundryCross = 0;
		}
		else {
			rec_len = ((unsigned int) (((unsigned char) buffer[DataDrive][pos+5]) << 8) + ((unsigned char) buffer[DataDrive][pos+4]));
			name_len = ((unsigned char) buffer[DataDrive][pos+6]);
			file_type = ((unsigned char) buffer[DataDrive][pos+7]);
			inode = ((unsigned int) (((unsigned char) buffer[DataDrive][pos+3]) << 24) + (((unsigned char) buffer[DataDrive][pos+2]) << 16) + (((unsigned char) buffer[DataDrive][pos+1]) << 8) + ((unsigned char) buffer[DataDrive][pos+0]));
		}

		if (name_len != NameLength) {
			pos += rec_len;
			if ((pos) >= 511) {
			   DataDrive = ReadRAID(CurrentDirectory+CDPos);
			   CDPos++;
			   pos=0;
			}
			if ((511 - pos) < 8) {
				BoundryCross = 1;
				BoundryIndex = 0;
				BoundryCrossPos = 511-pos;
				for (i=(512-pos); i; i--) {
					BoundryData[BoundryIndex] = buffer[DataDrive][pos];
					pos++;
					BoundryIndex++;
				}
				DataDrive = ReadRAID(CurrentDirectory+CDPos);
				CDPos++;
				pos=0;
				while (BoundryIndex < 8) {
					BoundryData[BoundryIndex] = buffer[DataDrive][pos];
					pos++;
					BoundryIndex++;
				}
				pos = -(BoundryCrossPos+1);

			}

			for (i = 0; i < 8; i++) {

				cout << uchar2hex(BoundryData[i]) << " ";
			}
			cout << "\n";
			continue;
		}

		if (inode == 0) {
			break;
		}


		for (i = 0; i < name_len; i++) {
			if ((pos+i) >= 511) {
				DataDrive = ReadRAID(CurrentDirectory+CDPos);
				CDPos++;
				pos = 0;
			}
			if (buffer[DataDrive][pos+8+i] == Directory[i]) {
				continue;
			}
			else {
				break;
			}
		}

		if (i == NameLength) {
			break;
		}

		pos += rec_len;
		if ((pos) >= 511) {
		   DataDrive = ReadRAID(CurrentDirectory+CDPos);
		   CDPos++;
		   pos=0;
		}

		if ((511 - pos) < 8) {
			BoundryCross = 1;
			BoundryIndex = 0;
			BoundryCrossPos = 511-pos;
			for (i=(512-pos); i; i--) {
				BoundryData[BoundryIndex] = buffer[DataDrive][pos];
				pos++;
				BoundryIndex++;
			}
			DataDrive = ReadRAID(CurrentDirectory+CDPos);
			CDPos++;
			pos=0;
			while (BoundryIndex < 8) {
				BoundryData[BoundryIndex] = buffer[DataDrive][pos];
				pos++;
				BoundryIndex++;
			}
			pos = -(BoundryCrossPos+1);

		}

		for (i = 0; i < 8; i++) {

			cout << uchar2hex(BoundryData[i]) << " ";
		}
		cout << "\n";


	} while (inode != 0);



	if (inode != 0) {

		int s_inodes_per_group = 8192;
		int s_inode_size = 256;
		int bg = (inode - 1) / s_inodes_per_group;		// Which Group Number
		int bgsector = bg / 16 + 8;					// = bg sector
		int bgoffset = bg % 16 * 32;

		cout << "bg: " << bg << " ipg: " << s_inodes_per_group << "\n";
		cout << "bgsector: " << bgsector << " bgoffset: " << bgoffset << "\n";
		int  DataDrive;

		DataDrive = ReadRAID(bgsector);

		unsigned int bg_inode_table_lo = (((unsigned char) buffer[DataDrive][bgoffset+11]) << 24) + (((unsigned char) buffer[DataDrive][bgoffset+10]) << 16) + (((unsigned char) buffer[DataDrive][bgoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+8]);
		cout << "Inode Table " << bg_inode_table_lo << "\n";

		int index = (inode - 1) % s_inodes_per_group;		// Which Item in that group
		int offset = index * s_inode_size;				// Offset of Item into INODE Table
		unsigned int InodeTableSector = bg_inode_table_lo * 8;		// sector of inode table from block group
		unsigned int InodeEntrySector = offset / 512 + InodeTableSector;	// sector of inode entry
		int EntryOffset = offset % 512;

		unsigned int LastSector;

		DataDrive = ReadRAID(InodeEntrySector);
		LastSector = InodeEntrySector;
		unsigned int filesize = 0;
		int extoffset = 40+EntryOffset;
		int extsector = bgsector;
		int extcount = 1;


		int eh_magic;
		int eh_entries;
		int eh_max, eh_depth;
		int eloc;
		unsigned int ee_block, ee_len, ee_start_hi, ee_start_lo, ei_block, ei_leaf_lo, ei_leaf_hi, ei_unused; 
		unsigned int datablocks, datasector;
		int idx = 0;
	        // loop until done with extents
		while (extcount > 0) {
	
		  DataDrive = ReadRAID(LastSector);
		  // read extent
		  if (idx == 0) {
			  eh_magic = ((int) (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
		  }
		  if (eh_magic == 0xF30A) {
			cout << "Extent Header\n";
		  	eh_entries = ((int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+2]));
			eh_max = ((int) (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
			eh_depth = ((int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+6]));
			extoffset += 12;
			extcount = eh_entries;
			datablocks = 0;
			eh_magic=0;
			if (eh_entries > 1 && eh_depth > 0) {
				cout << "Can't change directory multiple extent idx.";
				extcount = 0;
				break;
			}
			idx=1;
			continue;
		  }
		  else if (eh_depth == 1) { // extent idx
			ei_block = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+2]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
			ei_leaf_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+6]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
			ei_leaf_hi = ((int) (((unsigned char) buffer[DataDrive][extoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+8]));
			ei_unused = ((int) (((unsigned char) buffer[DataDrive][extoffset+11]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+10]));
			cout << "  ei_block: " << ei_block << " ei_leaf_lo: " << ei_leaf_lo << " ei_leaf_hi: " << ei_leaf_hi << " ei_unused: " << ei_unused << "\n";
			cout << "  Address: " << (ei_leaf_lo*8) << "\n";
			extsector = ei_leaf_lo*8;
			extoffset = 0;
			datablocks = 0;
			//extcount--;
			LastSector = extsector;
			DataDrive = ReadRAID(extsector);
			idx=0;
			continue;
		  }
		  else if (eh_depth == 0) { // extent leaf
			ee_block = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+2]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
			ee_len = ((int) (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
			ee_start_hi = ((int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+6]));
			ee_start_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+11]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+10]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+8]));
			cout << "  ee_block: " << ee_block << " ee_len: " << ee_len << " ee_start_hi: " << ee_start_hi << " ee_start_lo: " << ee_start_lo << "\n";
			cout << "  Address: " << (ee_start_lo*8) << "\n";
			datasector = ee_start_lo*8;
			datablocks = ee_len*8;
			extoffset += 12;
			extcount--;
			break; // pick first address and change directory
		  }
		  else {
			cout << "Nothing to do.";
			extcount = 0;
			datablocks = 0;
			continue;
		  }
	     }

	     CurrentDirectory = datasector;

	}
	else {
 	     cout << "Directory Not Found or Not A Directory";
	 }

    }
    else if (cmd.compare(0, 2, "ls") == 0 || cmd.compare(0, 3, "dir") == 0) {
	int  DataDrive;
	DataDrive = ReadRAID(CurrentDirectory);
	int CDPos = 1;

	int pos = 0;
	unsigned int name_len, rec_len;
	int i, file_type;
	int BoundryCross = 0;
	int BoundryCrossPos = 0;
	int BoundryIndex = 0;
	char BoundryData[8];

	do {

		if (BoundryCross == 1) {
			rec_len = ((unsigned int) (((unsigned char) BoundryData[5]) << 8) + ((unsigned char) BoundryData[4]));
			name_len = ((unsigned char) BoundryData[6]);
			file_type = ((unsigned char) BoundryData[7]);
			inode = ((unsigned int) (((unsigned char) BoundryData[3]) << 24) + (((unsigned char) BoundryData[2]) << 16) + (((unsigned char) BoundryData[1]) << 8) + ((unsigned char) BoundryData[0]));
			BoundryCross = 0;
		}
		else {
			rec_len = ((unsigned int) (((unsigned char) buffer[DataDrive][pos+5]) << 8) + ((unsigned char) buffer[DataDrive][pos+4]));
			name_len = ((unsigned char) buffer[DataDrive][pos+6]);
			file_type = ((unsigned char) buffer[DataDrive][pos+7]);
			inode = ((unsigned int) (((unsigned char) buffer[DataDrive][pos+3]) << 24) + (((unsigned char) buffer[DataDrive][pos+2]) << 16) + (((unsigned char) buffer[DataDrive][pos+1]) << 8) + ((unsigned char) buffer[DataDrive][pos+0]));
		}

		if (inode == 0) {
			break;
		}

		for (i = 0; i < name_len; i++) {
			if ((pos+i) >= 511) {
				DataDrive = ReadRAID(CurrentDirectory+CDPos);
				CDPos++;
				pos=0;
			}
			cout << buffer[DataDrive][pos+8+i];
		}

		if (file_type == 2) {
			cout << "/             " << inode << " " << file_type << " " << name_len << " " << rec_len << "\n";
		}
		else {
			cout << "              " << inode << " " << file_type << " " << name_len << " " << rec_len << "\n";
		}

		pos += rec_len;
		if ((pos) >= 511) {
		   DataDrive = ReadRAID(CurrentDirectory+CDPos);
		   CDPos++;
		   pos=0;
		}

		if ((511 - pos) < 8) {
			BoundryCross = 1;
			BoundryIndex = 0;
			BoundryCrossPos = 511-pos;
			for (i=(512-pos); i; i--) {
				BoundryData[BoundryIndex] = buffer[DataDrive][pos];
				pos++;
				BoundryIndex++;
			}
			DataDrive = ReadRAID(CurrentDirectory+CDPos);
			CDPos++;
			pos=0;
			while (BoundryIndex < 8) {
				BoundryData[BoundryIndex] = buffer[DataDrive][pos];
				pos++;
				BoundryIndex++;
			}
			pos = -(BoundryCrossPos+1);

		}
/*
		for (i = 0; i < 8; i++) {

			cout << uchar2hex(BoundryData[i]) << " ";
		}
		cout << "\n";
*/

	} while (inode != 0);

    }
    else if (cmd.compare(0, 2, "rl") == 0) {
      if (cmd.length() < 3) {
         cout << "Invalid input for command a\n";
         continue;
      }
      raidLevel = atoi(cmd.substr(3, cmd.length()-1).c_str());
    }
    else if (cmd.compare(0, 2, "ba") == 0) {
      if (cmd.length() < 3) {
         cout << "Invalid input for command a\n";
         continue;
      }
      BlockAdjust = atoi(cmd.substr(3, cmd.length()-1).c_str());
    }
    /* Hex and Ascii dump command */
    else if (cmd.compare(0, 1, "d") == 0 || cmd.compare(0, 1, "u") == 0) {
      if (cmd.length() > 2) {
        dumpIndex = atoi(cmd.substr(2, cmd.length()-1).c_str());
      }
      if (cmd.compare(0, 1, "d") == 0) {
        cout << "ASCII Dump " << dumpIndex << "\n";
      }
      if (cmd.compare(0, 1, "u") == 0) {
        cout << "HEX Dump " << dumpIndex << "\n";
      }

      zeroIgnoredDrive();
      readDrives(dumpIndex);
      createIgnoredDrive();

      /* Create ignored drive and print content of other drives */
      for (int j = 0; j < currentDrive; j++) {
        for(int i = 0; i < bufferSize; i++) {
          if (cmd.compare(0, 1, "d") == 0) {
            if (buffer[j][i] >= 0x20 && buffer[j][i] <= 0x7E) {
                cout << buffer[j][i];
            }
            else {
              cout << ".";
            }
          } 
          else if (cmd.compare(0, 1, "u") == 0) {
              cout << uchar2hex(buffer[j][i]) << " ";
          }
        }
        cout << "\n\n";
      }

      dumpIndex++;
      
    }
    else if (cmd.compare(0, 4, "pull") == 0) {
	if (cmd.length() > 5) {
		filename = cmd.substr(5, cmd.length()-1);
	        inode = atoi(cmd.substr(5, cmd.length()-1).c_str());
	}
        ofstream file(filename.c_str(), ios_base::binary);
        if(!file)
          throw(runtime_error(strerror(errno)));


	int s_inodes_per_group = 8192;
	int s_inode_size = 256;
	int bg = (inode - 1) / s_inodes_per_group;		// Which Group Number
	int bgsector = bg / 16 + 8;					// = bg sector
	int bgoffset = bg % 16 * 32;

	cout << "bg: " << bg << " ipg: " << s_inodes_per_group << "\n";
	cout << "bgsector: " << bgsector << " bgoffset: " << bgoffset << "\n";
	int  DataDrive;

	DataDrive = ReadRAID(bgsector);

	unsigned int bg_inode_table_lo = (((unsigned char) buffer[DataDrive][bgoffset+11]) << 24) + (((unsigned char) buffer[DataDrive][bgoffset+10]) << 16) + (((unsigned char) buffer[DataDrive][bgoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+8]);
	cout << "Inode Table " << bg_inode_table_lo << "\n";

	int index = (inode - 1) % s_inodes_per_group;		// Which Item in that group
	int offset = index * s_inode_size;				// Offset of Item into INODE Table
	unsigned int InodeTableSector = bg_inode_table_lo * 8;		// sector of inode table from block group
	unsigned int InodeEntrySector = offset / 512 + InodeTableSector;	// sector of inode entry
	int EntryOffset = offset % 512;

	unsigned int LastSector;

	DataDrive = ReadRAID(InodeEntrySector);
	LastSector = InodeEntrySector;
	unsigned int filesize = 0;
	int extoffset = 40+EntryOffset;
	int extsector = bgsector;
	int extcount = 1;

	filesize = ((unsigned int) (((unsigned char) buffer[DataDrive][EntryOffset+7]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+6]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+5]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+4]));
	cout << "File Size: " << filesize << "\n";

	int eh_magic;
	int eh_entries;
	int eh_max, eh_depth;
	int eloc;
	unsigned int ee_block, ee_len, ee_start_hi, ee_start_lo, ei_block, ei_leaf_lo, ei_leaf_hi, ei_unused; 
	unsigned int datablocks, datasector;
	int idx = 0;
        // loop until done with extents
	while (extcount > 0) {

	  DataDrive = ReadRAID(LastSector);
	  // read extent
	  if (idx == 0) {
		  eh_magic = ((int) (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
	  }
	  if (eh_magic == 0xF30A) {
		//cout << "Extent Header\n";
	  	eh_entries = ((int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+2]));
		eh_max = ((int) (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
		eh_depth = ((int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+6]));
		cout << "  eh_magic: " << eh_magic << " eh_entries: " << eh_entries << " eh_max: " << eh_max << " eh_depth: " << eh_depth << "\n";
		extoffset += 12;
		extcount = eh_entries;
		datablocks = 0;
		eh_magic=0;
		if (eh_entries > 1 && eh_depth > 0) {
			cout << "Can't pull file multiple extent idx.";
			extcount = 0;
			break;
		}
		idx=1;
		continue;
	  }
	  else if (eh_depth == 1) { // extent idx
		ei_block = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+2]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
		ei_leaf_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+6]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
		ei_leaf_hi = ((int) (((unsigned char) buffer[DataDrive][extoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+8]));
		ei_unused = ((int) (((unsigned char) buffer[DataDrive][extoffset+11]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+10]));
		cout << "  ei_block: " << ei_block << " ei_leaf_lo: " << ei_leaf_lo << " ei_leaf_hi: " << ei_leaf_hi << " ei_unused: " << ei_unused << "\n";
		cout << "  Address: " << (ei_leaf_lo*8) << "\n";
		extsector = ei_leaf_lo*8;
		extoffset = 0;
		datablocks = 0;
		//extcount--;
		LastSector = extsector;
		DataDrive = ReadRAID(extsector);
		idx=0;
		continue;
	  }
	  else if (eh_depth == 0) { // extent leaf
		ee_block = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+3]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+2]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+1]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+0]));
		ee_len = ((int) (((unsigned char) buffer[DataDrive][extoffset+5]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+4]));
		ee_start_hi = ((int) (((unsigned char) buffer[DataDrive][extoffset+7]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+6]));
		ee_start_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][extoffset+11]) << 24) + (((unsigned char) buffer[DataDrive][extoffset+10]) << 16) + (((unsigned char) buffer[DataDrive][extoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][extoffset+8]));
		cout << "  ee_block: " << ee_block << " ee_len: " << ee_len << " ee_start_hi: " << ee_start_hi << " ee_start_lo: " << ee_start_lo << "\n";
		cout << "  Address: " << (ee_start_lo*8) << "\n";
		datasector = ee_start_lo*8;
		datablocks = ee_len*8;
		extoffset += 12;
		extcount--;
	  }
	  else {
		cout << "Nothing to do.";
		extcount = 0;
		datablocks = 0;
		continue;
	  }

	  while (datablocks > 0 && filesize > 0) {
	        cout << "Filesize: " << filesize << " DataBlocks: " << datablocks << "\r" << std::flush;
		DataDrive = ReadRAID(datasector);
		if (filesize < 512) {
			file.write(&buffer[DataDrive][0], filesize);
			if (!file) {
				cout << "Write Error\n";
			}
			filesize = 0;
		}
		else {
			file.write(&buffer[DataDrive][0], 512);
			if (!file) {
				cout << "Write Error\n";
			}
			filesize -= 512;
		}
		datablocks--;
		datasector++;
	  }

        }
	if (filesize > 0 || datablocks > 7 || extcount > 0) {
		cout << "Pull failed to complete file extraction.\n";
		cout << " filesize: " << filesize << " datablocks: " << datablocks <<  " extcount: " << extcount << "\n";
	}
	file.close();
    }
    else if (cmd.compare(0, 5, "inode") == 0) {
      if (cmd.length() > 6) {
        inode = atoi(cmd.substr(6, cmd.length()-1).c_str());
      }

      cout << "INODE " << inode << "\n";

	//int s_inodes_per_group = 8192;
	int s_inodes_per_group = 8192;
	int s_inode_size = 256;
	int bg = (inode - 1) / s_inodes_per_group;		// Which Group Number
	int bgsector = bg / 16 + 8;					// = bg sector
	int bgoffset = bg % 16 * 32;

	cout << "bg: " << bg << " ipg: " << s_inodes_per_group << "\n";
	cout << "bgsector: " << bgsector << " bgoffset: " << bgoffset << "\n";
	int  DataDrive;

	DataDrive = ReadRAID(bgsector);

	unsigned int bg_inode_table_lo = (((unsigned char) buffer[DataDrive][bgoffset+11]) << 24) + (((unsigned char) buffer[DataDrive][bgoffset+10]) << 16) + (((unsigned char) buffer[DataDrive][bgoffset+9]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+8]);


	cout << "Inode Table " << bg_inode_table_lo << "\n";

	int index = (inode - 1) % s_inodes_per_group;		// Which Item in that group
	int offset = index * s_inode_size;				// Offset of Item into INODE Table
	unsigned int InodeTableSector = bg_inode_table_lo * 8;		// sector of inode table from block group
	unsigned int InodeEntrySector = offset / 512 + InodeTableSector;	// sector of inode entry
	int EntryOffset = offset % 512;

	DataDrive = ReadRAID(InodeEntrySector);

	cout << "index: " << index << " offset " << offset << "\n";
	cout << "InodeEntrySector: " << InodeEntrySector << " EntryOffset: " << EntryOffset << "\n";
	cout << "InodeTableSector: " << InodeTableSector << "\n";

	cout << "i_mode: " << ((((unsigned char) buffer[DataDrive][EntryOffset+1]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+0])) << "\n";
	cout << "i_uid: " << ((((unsigned char) buffer[DataDrive][EntryOffset+3]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+2])) << "\n";
	cout << "i_size_lo: " << ((((unsigned char) buffer[DataDrive][EntryOffset+7]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+6]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+5]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+4]));
	cout << ", i_size_high: " << ((((unsigned char) buffer[DataDrive][EntryOffset+111]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+110]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+109]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+108])) << "\n";
	//uint64_t i_size = ((((unsigned char) buffer[DataDrive][EntryOffset+111]) << 56) + (((unsigned char) buffer[DataDrive][EntryOffset+110]) << 48) + (((unsigned char) buffer[DataDrive][EntryOffset+109]) << 40) + (((unsigned char) buffer[DataDrive][bgoffset+108]) << 32) + (((unsigned char) buffer[DataDrive][EntryOffset+7]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+6]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+5]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+4]));
	//cout <<  "i_size: " << i_size << "\n";
	cout << "i_atime: " << ((((unsigned char) buffer[DataDrive][EntryOffset+11]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+10]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+9]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+8]));
	cout << ", i_atime_extra: " << ((((unsigned char) buffer[DataDrive][EntryOffset+143]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+142]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+141]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+140])) << "\n";
	cout << "i_ctime: " << ((((unsigned char) buffer[DataDrive][EntryOffset+15]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+14]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+13]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+12]));
	cout << ", i_ctime_extra: " << ((((unsigned char) buffer[DataDrive][EntryOffset+135]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+134]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+133]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+132])) << "\n";
	cout << "i_mtime: " << ((((unsigned char) buffer[DataDrive][EntryOffset+19]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+18]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+17]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+16]));
	cout << ", i_mtime_extra: " << ((((unsigned char) buffer[DataDrive][EntryOffset+139]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+138]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+137]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+136])) << "\n";
	cout << "i_dtime: " << ((((unsigned char) buffer[DataDrive][EntryOffset+23]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+22]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+21]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+20])) << "\n";
	cout << "i_gid: " << ((((unsigned char) buffer[DataDrive][EntryOffset+25]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+24])) << "\n";
	cout << "i_links_count: " << ((((unsigned char) buffer[DataDrive][EntryOffset+27]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+26])) << "\n";
	cout << "i_blocks_lo: " << ((((unsigned char) buffer[DataDrive][EntryOffset+31]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+30]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+29]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+28])) << "\n";
	cout << "i_flags: " << ((((unsigned char) buffer[DataDrive][EntryOffset+35]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+34]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+33]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+32])) << "\n";
	cout << "osd1: " << uchar2hex(buffer[DataDrive][EntryOffset+36]) << uchar2hex(buffer[DataDrive][EntryOffset+37]) << uchar2hex(buffer[DataDrive][EntryOffset+38]) << uchar2hex(buffer[DataDrive][EntryOffset+39]) << "\n";
	cout << "i_block: " << uchar2hex(buffer[DataDrive][EntryOffset+40]) << uchar2hex(buffer[DataDrive][EntryOffset+41]) << uchar2hex(buffer[DataDrive][EntryOffset+42]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+43]) << uchar2hex(buffer[DataDrive][EntryOffset+44]) << uchar2hex(buffer[DataDrive][EntryOffset+45]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+46]) << uchar2hex(buffer[DataDrive][EntryOffset+47]) << uchar2hex(buffer[DataDrive][EntryOffset+48]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+49]);
        cout << uchar2hex(buffer[DataDrive][EntryOffset+50]) << uchar2hex(buffer[DataDrive][EntryOffset+51]) << uchar2hex(buffer[DataDrive][EntryOffset+52]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+53]) << uchar2hex(buffer[DataDrive][EntryOffset+54]) << uchar2hex(buffer[DataDrive][EntryOffset+55]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+56]) << uchar2hex(buffer[DataDrive][EntryOffset+57]) << uchar2hex(buffer[DataDrive][EntryOffset+58]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+59]);
        cout << uchar2hex(buffer[DataDrive][EntryOffset+60]) << uchar2hex(buffer[DataDrive][EntryOffset+61]) << uchar2hex(buffer[DataDrive][EntryOffset+62]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+63]) << uchar2hex(buffer[DataDrive][EntryOffset+64]) << uchar2hex(buffer[DataDrive][EntryOffset+65]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+66]) << uchar2hex(buffer[DataDrive][EntryOffset+67]) << uchar2hex(buffer[DataDrive][EntryOffset+68]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+69]);
        cout << uchar2hex(buffer[DataDrive][EntryOffset+70]) << uchar2hex(buffer[DataDrive][EntryOffset+71]) << uchar2hex(buffer[DataDrive][EntryOffset+72]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+73]) << uchar2hex(buffer[DataDrive][EntryOffset+74]) << uchar2hex(buffer[DataDrive][EntryOffset+75]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+76]) << uchar2hex(buffer[DataDrive][EntryOffset+77]) << uchar2hex(buffer[DataDrive][EntryOffset+78]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+79]);
        cout << uchar2hex(buffer[DataDrive][EntryOffset+80]) << uchar2hex(buffer[DataDrive][EntryOffset+81]) << uchar2hex(buffer[DataDrive][EntryOffset+82]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+83]) << uchar2hex(buffer[DataDrive][EntryOffset+84]) << uchar2hex(buffer[DataDrive][EntryOffset+85]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+86]) << uchar2hex(buffer[DataDrive][EntryOffset+87]) << uchar2hex(buffer[DataDrive][EntryOffset+88]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+89]);
        cout << uchar2hex(buffer[DataDrive][EntryOffset+90]) << uchar2hex(buffer[DataDrive][EntryOffset+91]) << uchar2hex(buffer[DataDrive][EntryOffset+92]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+93]) << uchar2hex(buffer[DataDrive][EntryOffset+94]) << uchar2hex(buffer[DataDrive][EntryOffset+95]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+96]) << uchar2hex(buffer[DataDrive][EntryOffset+97]) << uchar2hex(buffer[DataDrive][EntryOffset+98]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+99]) << "\n";

	int eh_magic = ((int) (((unsigned char) buffer[DataDrive][EntryOffset+41]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+40]));
	int eh_entries=0;
	int eh_max, eh_depth;
	int eloc = 0;
	unsigned int ee_block, ee_len, ee_start_hi, ee_start_lo, ei_block, ei_leaf_lo, ei_leaf_hi, ei_unused; 
	if (eh_magic == 0xF30A) {
		//cout << "Extent Header Found!\n";
		eh_entries = ((int) (((unsigned char) buffer[DataDrive][EntryOffset+43]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+42]));
		eh_max = ((int) (((unsigned char) buffer[DataDrive][EntryOffset+45]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+44]));
		eh_depth = ((int) (((unsigned char) buffer[DataDrive][EntryOffset+47]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+46]));
		if (eh_depth == 0) { // leaf node
			eloc=EntryOffset+52;
			for (int i = 0; i < eh_entries; i++) {
				ee_block = ((unsigned int) (((unsigned char) buffer[DataDrive][eloc+3]) << 24) + (((unsigned char) buffer[DataDrive][eloc+2]) << 16) + (((unsigned char) buffer[DataDrive][eloc+1]) << 8) + ((unsigned char) buffer[DataDrive][eloc+0]));
				ee_len = ((int) (((unsigned char) buffer[DataDrive][eloc+5]) << 8) + ((unsigned char) buffer[DataDrive][eloc+4]));
				ee_start_hi = ((int) (((unsigned char) buffer[DataDrive][eloc+7]) << 8) + ((unsigned char) buffer[DataDrive][eloc+6]));
				ee_start_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][eloc+11]) << 24) + (((unsigned char) buffer[DataDrive][eloc+10]) << 16) + (((unsigned char) buffer[DataDrive][eloc+9]) << 8) + ((unsigned char) buffer[DataDrive][eloc+8]));
				eloc += 12;
				cout << "  ee_block: " << ee_block << " ee_len: " << ee_len << " ee_start_hi: " << ee_start_hi << " ee_start_lo: " << ee_start_lo << "\n";

				//uint64_t ee_start = ( (((unsigned char) buffer[DataDrive][eloc+7]) << 40) + (((unsigned char) buffer[DataDrive][eloc+6]) << 32) + (((unsigned char) buffer[DataDrive][eloc+11]) << 24) + (((unsigned char) buffer[DataDrive][eloc+10]) << 16) + (((unsigned char) buffer[DataDrive][eloc+9]) << 8) + ((unsigned char) buffer[DataDrive][eloc+8]));
				//cout <<  "i_size: " << i_size << "\n";

				cout << "  Address: " << (ee_start_lo*8) << "\n";
			}
		}
		else {	// idx node
			eloc=EntryOffset+52;
			for (int i = 0; i < eh_entries; i++) {
				ei_block = ((unsigned int) (((unsigned char) buffer[DataDrive][eloc+3]) << 24) + (((unsigned char) buffer[DataDrive][eloc+2]) << 16) + (((unsigned char) buffer[DataDrive][eloc+1]) << 8) + ((unsigned char) buffer[DataDrive][eloc+0]));
				ei_leaf_lo = ((unsigned int) (((unsigned char) buffer[DataDrive][eloc+7]) << 24) + (((unsigned char) buffer[DataDrive][eloc+6]) << 16) + (((unsigned char) buffer[DataDrive][eloc+5]) << 8) + ((unsigned char) buffer[DataDrive][eloc+4]));
				ei_leaf_hi = ((int) (((unsigned char) buffer[DataDrive][eloc+9]) << 8) + ((unsigned char) buffer[DataDrive][eloc+8]));
				ei_unused = ((int) (((unsigned char) buffer[DataDrive][eloc+11]) << 8) + ((unsigned char) buffer[DataDrive][eloc+10]));
				eloc += 12;
				cout << "  ei_block: " << ei_block << " ei_leaf_lo: " << ei_leaf_lo << " ei_leaf_hi: " << ei_leaf_hi << " ei_unused: " << ei_unused << "\n";
				cout << "  Address: " << (ei_leaf_lo*8) << "\n";
			}
		}
	}

	cout << "i_generation: " << ((((unsigned char) buffer[DataDrive][EntryOffset+103]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+102]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+101]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+100])) << "\n";
	cout << "i_file_acl_lo: " << ((((unsigned char) buffer[DataDrive][EntryOffset+107]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+106]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+105]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+104])) << "\n";
	cout << "i_obso_faddr: " << ((((unsigned char) buffer[DataDrive][EntryOffset+115]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+114]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+113]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+112])) << "\n";
	cout << "osd2: " << uchar2hex(buffer[DataDrive][EntryOffset+116]) << uchar2hex(buffer[DataDrive][EntryOffset+117]) << uchar2hex(buffer[DataDrive][EntryOffset+118]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+119]) << uchar2hex(buffer[DataDrive][EntryOffset+120]) << uchar2hex(buffer[DataDrive][EntryOffset+121]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+122]) << uchar2hex(buffer[DataDrive][EntryOffset+123]) << uchar2hex(buffer[DataDrive][EntryOffset+124]);
	cout << uchar2hex(buffer[DataDrive][EntryOffset+125]) << uchar2hex(buffer[DataDrive][EntryOffset+126]) << uchar2hex(buffer[DataDrive][EntryOffset+127]) << "\n";
	cout << "i_extra_isize: " << ((((unsigned char) buffer[DataDrive][EntryOffset+129]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+128])) << "\n";
	cout << "i_checksum_hi: " << ((((unsigned char) buffer[DataDrive][EntryOffset+131]) << 8) + ((unsigned char) buffer[DataDrive][EntryOffset+130])) << "\n";
	cout << "i_crtime: " << ((((unsigned char) buffer[DataDrive][EntryOffset+147]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+146]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+145]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+144]));
	cout << ", i_crtime_extra: " << ((unsigned int) (((unsigned char) buffer[DataDrive][EntryOffset+151]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+150]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+149]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+148])) << "\n";
	cout << "i_version_hi: " << ((((unsigned char) buffer[DataDrive][EntryOffset+155]) << 24) + (((unsigned char) buffer[DataDrive][EntryOffset+154]) << 16) + (((unsigned char) buffer[DataDrive][EntryOffset+153]) << 8) + ((unsigned char) buffer[DataDrive][bgoffset+152])) << "\n";



        inode++;
	

    }
    else if (cmd.compare(0, 2, "rd") == 0 || cmd.compare(0, 2, "ru") == 0) {
      if (cmd.length() > 3) {
        dumpRaidSector = atoi(cmd.substr(3, cmd.length()-1).c_str());
      }
      if (cmd.compare(0, 2, "rd") == 0) {
        cout << "ASCII RAID Dump " << dumpRaidSector << "\n";
      }
      if (cmd.compare(0, 2, "ru") == 0) {
        cout << "HEX RAID Dump " << dumpRaidSector << "\n";
      }


      int DataDrive;

	DataDrive = ReadRAID(dumpRaidSector);
	cout << "Drive: " << DataDrive << "\n";


        //Read DriveBlock from DataDrive
        for(int i = 0; i < bufferSize; i++) {
          if (cmd.compare(0, 2, "rd") == 0) {
            if (buffer[DataDrive][i] >= 0x20 && buffer[DataDrive][i] <= 0x7E) {
                cout << buffer[DataDrive][i];
            }
            else {
              cout << ".";
            }
          } 
          else if (cmd.compare(0, 2, "ru") == 0) {
              cout << uchar2hex(buffer[DataDrive][i]) << " ";
          }
        }
        cout << "\n\n";

	dumpRaidSector++;



    }
    /* List drives command */
    else if (cmd.compare(0, 1, "l") == 0) {
      int i = 0;
      cout << "List\n";
      for(i= 0; i < currentDrive; i++) {
        if (drives[i].compare("") == 0) {
          cout << i << " [missing]\n";
        }
        else {
          cout << i << " " << drives[i] << "\n";
        }
      }
      if (i == 0) {
        cout << "No drives have been added";
      }
    }
    /* Help command */
    else if (cmd.compare("h") == 0) {
      help();
    }
    /*
    else if (cmd.compare(0, 1, "o") == 0) {
      cout << "Order\n";
    }
    */

    else if (cmd.compare(0, 2, "ss") == 0) {
      if (cmd.length() > 3) {
        searchIndex = atoi(cmd.substr(3, cmd.length()-1).c_str());
        searchIndexFlag = 1;
      }
    }
    /* Seach disk command */
    else if (cmd.compare(0, 1, "s") == 0) {
      int activeSearch = 1;
      string searchString = cmd.substr(2, cmd.length()-1);
      int stringIndex = 0;
      int bufferCount = 0;
      int bytesPerSec = 0;

      if (searchIndexFlag == 0) {
        searchIndex = 0;
      }
      searchIndexFlag = 0;

      cout << "Searching for " << searchString << "\n";
      //cout << "Press any key to stop the search\n";


      struct timeval tv;
      setTimer(tv,5); //set up a delay timer

      while(activeSearch) {

        if (checkTimer(tv,1)==1) {
          bytesPerSec = bufferCount*bufferSize;
          
          bufferCount = 0;
        }

        zeroIgnoredDrive();

        readDrives(searchIndex);
        cout << "Block " << searchIndex << "/" << dsize/bufferSize << " [" << bytesPerSec << "B/Sec]   \r" << std::flush;
        bufferCount += currentDrive+1;
	createIgnoredDrive();

        // search each buffer
        for(int i = 0; i < currentDrive; i++) {
          for(int j = 0; j < bufferSize; j++) {
            if (buffer[i][j] == searchString[stringIndex]) {
              if (stringIndex == searchString.length()) {
                cout << "Found: Disk=" << drives[i] << " Block=" << searchIndex << "                    \n";
                stringIndex = 0;
                break;
              }
              stringIndex++;
            }
            else {
              stringIndex = 0;
            }
          }
        }
        searchIndex++;
      } // while(activeSearch)
    }
    /* Quit command */
    else if (cmd.compare("q") == 0) {
      run = 0;
    }
  }
}

