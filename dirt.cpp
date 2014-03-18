/**********************************************
 * Disk Image Recovery Tool (DIRT)
 * -------------------------------------------
 * Created By        Marcus Kelly
 *    Contact        nexuslite@gmail.com
 * Created On        Feb 23, 2014
 * Updated On        Mar 14, 2014
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


using namespace std;

#define BUF_SIZE 512
#define MAX_SEEK 2147483648

string VersionHigh = "0";
string VersionLow = "1a";
int raidLevel = 5;
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

void help() {
  cout << "---------------------------------------\n";
  cout << "  a=/dev/sdb           Add Drive\n";
  cout << "  bs                   Block Size Selection Menu\n";
  cout << "  d                    Dump Drive Block as ASCII\n";
  cout << "  d=[block]            Dump Drive Block as ASCII\n";
  //cout << "  f=[fs]               Change File System [ext4, fat, ntfs]
  cout << "  h                    Help\n";
  cout << "  inode                Display Next Inode\n";
  cout << "  inode=[inode]        Display Inode\n";
  cout << "  l                    List Drives\n";
  //cout << "  o                    Order Drives\n";
  //cout << "  rl=[level]            Change Raid Level [0, 1, 2, 3, 4, 5, 6]
  cout << "  rbs                  Raid Block Size Selection Menu\n";
  cout << "  rd                   Dump Raid Block as ASCII\n";
  cout << "  rd=[block]           Dump Raid Block as ASCII\n";
  cout << "  rsb=[block]          Raid Start in Drive Block (dependent on Block Size)\n";
  cout << "  rs=[string]          Raid Search\n";
  cout << "  ru                   Dump Raid Block as HEX\n";
  cout << "  ru=[block]           Dump Raid Block as HEX\n";
  cout << "  stat                 Display current configuration\n";
  cout << "  s=[string]           Search\n";
  cout << "  ss=[block]           Start search from block\n";
  cout << "  u                    Dump Drive Block as HEX\n";
  cout << "  u=block              Dump Drive Block as HEX\n";
  cout << "  q                    Quit\n";
  cout << "---------------------------------------\n";

}

void conf() {
  cout << "---------------------------------------\n";
  cout << "Block Size: " << bufferSize << "\n";
  cout << "Raid Level: " << raidLevel << "\n";
  cout << "Raid Start Block: " << raidStartBlock << "\n";
  cout << "Raid Block Size: " << raidBlockSize << "\n";
  cout << "File System: " << fileSystem << "\n";
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
      cout << "1:  64K\n";
      cout << "2: 128K\n";
      cout << "3: 256K\n";
      cout << "4: 512K\n";
      cout << ":";
      cin >> opt;
      if (opt.compare(0, 1, "1") == 0) {
        raidBlockSize = 65536;
      }
      if (opt.compare(0, 1, "2") == 0) {
        raidBlockSize = 131072;
      }
      if (opt.compare(0, 1, "3") == 0) {
        raidBlockSize = 262144;
      }
      if (opt.compare(0, 1, "4") == 0) {
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
        conf();
    }
    else if (cmd.compare(0, 3, "rbs") == 0) {
        setRaidBlockSize();
    }
    else if (cmd.compare(0, 3, "rsb") == 0) {
      if (cmd.length() < 4) {
         cout << "Invalid input for command a\n";
         continue;
      }
      raidStartBlock = atoi(cmd.substr(4, cmd.length()-1).c_str());
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


      // combine sectors using raid 5 algorithm
      dumpIndex++;
      
    }
    else if (cmd.compare(0, 5, "inode") == 0) {
      if (cmd.length() > 6) {
        inode = atoi(cmd.substr(6, cmd.length()-1).c_str());
      }

      cout << "INODE " << inode << "\n";

	int s_inodes_per_group = 8192;
	int s_inode_size = 256;
	int bg = (inode - 1) / s_inodes_per_group;		// Which Group Number
	int bgsector = bg / 16 + 8;					// = bg sector
	int bgoffset = bg % 16 * 32;

        int TotalDisks = currentDrive+1;
        int DataDisks = currentDrive;
	int SectorsPerBlock = raidBlockSize/bufferSize;
	unsigned int BlockNumber = bgsector/SectorsPerBlock;
        unsigned int DriveBlock = bgsector%(SectorsPerBlock)+(BlockNumber/(DataDisks-1)*SectorsPerBlock)+raidStartBlock;
	int BlockRadius = BlockNumber%(DataDisks*TotalDisks);
	int ParityDrive = ((TotalDisks-1)-(int) (BlockRadius/DataDisks))-1;
	int DataDrive = (BlockRadius%(DataDisks-1));
	if (ParityDrive <= DataDrive) {
	    DataDrive++;
	}

        zeroIgnoredDrive();
        readDrives(DriveBlock);
        createIgnoredDrive();

	int bg_inode_table_lo = (((int) buffer[DataDrive][bgoffset+11]) << 24) + (((int) buffer[DataDrive][bgoffset+10]) << 16) + (((int) buffer[DataDrive][bgoffset+9]) << 8) + ((int) buffer[DataDrive][bgoffset+8]);
	cout << "Inode Table " << bg_inode_table_lo << "\n";

	int index = (inode - 1) % s_inodes_per_group;		// Which Item in that group
	int offset = index * s_inode_size;				// Offset of Item into INODE Table
	int InodeTableSector = bg_inode_table_lo * 8;		// sector of inode table from block group
	int InodeEntrySector = offset / 512 + InodeTableSector;	// sector of inode entry
	int EntryOffset = offset % 512;



	BlockNumber = InodeEntrySector/SectorsPerBlock;
        DriveBlock = InodeEntrySector%(SectorsPerBlock)+(BlockNumber/(DataDisks-1)*SectorsPerBlock)+raidStartBlock;
	BlockRadius = BlockNumber%(DataDisks*TotalDisks);
	ParityDrive = ((TotalDisks-1)-(int) (BlockRadius/DataDisks))-1;
	DataDrive = (BlockRadius%(DataDisks-1));
	if (ParityDrive <= DataDrive) {
	    DataDrive++;
	}

        zeroIgnoredDrive();
        readDrives(DriveBlock);
        createIgnoredDrive();

	cout << "index: " << index << " offset " << offset << "\n";
	cout << "InodeEntrySector: " << InodeEntrySector << " EntryOffset: " << EntryOffset << "\n";
	cout << "InodeTableSector: " << InodeTableSector << "\n";


	cout << "i_mode: " << ((((int) buffer[DataDrive][EntryOffset+1]) << 8) + ((int) buffer[DataDrive][EntryOffset+0])) << "\n";
	cout << "i_uid: " << ((((int) buffer[DataDrive][EntryOffset+3]) << 8) + ((int) buffer[DataDrive][EntryOffset+2])) << "\n";
	cout << "i_size_lo: " << ((((int) buffer[DataDrive][EntryOffset+7]) << 24) + (((int) buffer[DataDrive][EntryOffset+6]) << 16) + (((int) buffer[DataDrive][EntryOffset+5]) << 8) + ((int) buffer[DataDrive][bgoffset+4])) << "\n";

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
	cout << uchar2hex(buffer[DataDrive][EntryOffset+99]);

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


        int TotalDisks = currentDrive+1;
        int DataDisks = currentDrive;
	int SectorsPerBlock = raidBlockSize/bufferSize;
	unsigned int BlockNumber = dumpRaidSector/SectorsPerBlock;
        unsigned int DriveBlock = dumpRaidSector%(SectorsPerBlock)+(BlockNumber/(DataDisks-1)*SectorsPerBlock)+raidStartBlock;
	int BlockRadius = BlockNumber%(DataDisks*TotalDisks);
	int ParityDrive = ((TotalDisks-1)-(int) (BlockRadius/DataDisks))-1;
	int DataDrive = (BlockRadius%(DataDisks-1));
	if (ParityDrive <= DataDrive) {
	    DataDrive++;
	}

      cout << "SectorsPerBlock: " << SectorsPerBlock << ", BlockNumber: " << BlockNumber << ", BlockRadius: " << BlockRadius << ", ParityDrive: " << ParityDrive << "\n";


      cout << "DriveBlock: " << DriveBlock << ", DataDrive: " << DataDrive << "\n";

      zeroIgnoredDrive();
      readDrives(DriveBlock);
      createIgnoredDrive();
      
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

