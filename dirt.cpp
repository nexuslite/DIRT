/**********************************************
 * Disk Image Recovery Tool (DIRT)
 * -------------------------------------------
 * Created By        Marcus Kelly
 *    Contact        nexuslite@gmail.com
 * Created On        Feb 23, 2014
 * Updated On        Apr 03, 2014
 * -------------------------------------------
 * Copyright 2014 Marcus Kelly
 **********************************************/
#include <stdlib.h> /* atoi */
#include <sys/time.h> /* SetTimer & CheckTimer */
#include <stdio.h>

#include <sstream>  /* uchar2hex */
#include <iomanip>  /* uchar2hex */

#include "ext.h"
#include "drive.h"

using namespace std;

string VersionHigh = "0";
string VersionMid = "3";
string VersionLow = "2a";
string fileSystem = "ext4";
int dumpIndex = 0;
int searchIndex = 0;
int searchIndexFlag = 0;
int inode;
string filename;
bool DrivesDisplayAll = true;

void help() {


	cout << "---------------------------------------\n";

	//cout << "  f=[fs]               Change File System [ext4, fat, ntfs]
	cout << "  h                    Help\n";
	cout << "  reset                Reset Configurations\n";
	cout << "  stat                 Display current configuration\n";
	cout << "  w                    Quit and Save Configuration\n";
	cout << "  q                    Quit\n";

	cout << "  add=[drive]          Add Drive\n";
	cout << "  list                 List Drives\n";
	cout << "  swap			Swap Drives\n";
	cout << "  delete		Delete Drives\n";
	cout << "  d[=block]            Dump Drive Block as ASCII (Next if no number given)\n";
	cout << "  u[=block]            Dump Drive Block as HEX (Next if no number given)\n";
	cout << "  bs                   Block Size Selection Menu\n";
	cout << "  ddaq                  Display Data from All Drives ON/OFF\n"; 
	cout << "  ba=[block]           Raid Block Adjust, Padding for Data Start\n";
	cout << "  rbs                  Raid Block Size Selection Menu\n";
	cout << "  rl=[level]           Set Raid Level [0, 1, 2, 3, 4, 5, 6]\n";
	cout << "  rsb=[block]          Raid Start in Drive Block (dependent on Block Size)\n";
	cout << "  s=[string]           Search\n";
	cout << "  ss=[block]           Start search from block\n";

	//cout << "  ds                   Search For All Directories\n"
	cout << "  inode[=inode]        Display Inode (Next if no number given)\n";
	cout << "  sd=[sector]          Set Directory\n";
	cout << "  dir | ls             List Directory\n";
	cout << "  cd=[name]            Change Directory\n";
	cout << "  fd=[name]            File Dump (Saves file in current system directory)\n";
        cout << "                           * will dump all files in a directory\n";
	cout << "  pull=[inode]         Pull from drive and save in current directory as inode #\n";
	//cout << "  root                 Find Root Directory\n";

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
	if(ctv.tv_sec > tv.tv_sec) {
		gettimeofday(&tv,NULL);
		tv.tv_sec+=sec;
		return 1;
	}
	else {
		return 0;
	}
}

int LoadConfig(Drive* drive, Ext* ext) {
	int BufIndex = 0;
	int CmdIndex = 0;
	int ValIndex = 0;
	char Buffer[512];
	char Cmd[64];
	char Val[256];
	ifstream file("dirt.cnf");
	if(!file) {
		//cout << "\nCould not open config file.\n";
		return 1;
	}

	file.getline(Buffer, 512);
	if (!file) {
		cout << "\nCould not read config file.\n";
		return 1;
	}

	cout << "\n";

	bool getcmd = true;
	bool getval = false;
	for(int i = 0; i < sizeof(Buffer); i++) {
		if (Buffer[BufIndex] == '\0') {
			break;
		}
		else if (Buffer[BufIndex] == ';') {
			Val[ValIndex] = '\0';
			if (strcmp(Cmd, "Drive") == 0) {
				cout << "Adding Device: " << Val << "\n";
				drive->AddDevice(Val);
			}
			if (strcmp(Cmd, "RaidLevel") == 0) {
				cout << "Raid Level: " << Val << "\n";
				drive->SetRaidLevel(atoi(Val));
			}
			if (strcmp(Cmd, "RaidBlockAdjust") == 0) {
				cout << "Raid Block Adjust: " << Val << "\n";
				drive->SetRaidBlockAdjust(atoi(Val));
			}
			if (strcmp(Cmd, "RaidBlockSize") == 0) {
				cout << "Raid Block Size: " << Val << "\n";
				drive->SetRaidBlockSize(atoi(Val));
			}
			if (strcmp(Cmd, "BufferSize") == 0) {
				cout << "Buffer Size: " << Val << "\n";
				drive->SetBufferSize(atoi(Val));
			}
			if (strcmp(Cmd, "DrivesDisplayAll") == 0) {
				if (atoi(Val) == 0) {
					cout << "Drives Display All: OFF\n";
					DrivesDisplayAll = false;
				}
				else {
					cout << "Drives Display All: ON\n";
					DrivesDisplayAll = true;
				}
			}
			if (strcmp(Cmd, "Root") == 0) {
				cout << "Root: " << Val << "\n";
				ext->SetCurrentDirectory(atoi(Val));
			}
			Buffer[0] = '\0';
			file.getline(Buffer, 512);
			if (!file) {
				break;
			}
			getcmd = true;
			getval = false;
			BufIndex = 0;
			CmdIndex = 0;
			ValIndex = 0;
			Cmd[0] = '\0';
			Val[0] = '\0';
			continue;
		}
		else if (Buffer[BufIndex] == ':') {
			Cmd[CmdIndex] = '\0';
			BufIndex++;
			getcmd = false;
			getval = true;
			continue;
		}
		else if (getcmd == true) {
			Cmd[CmdIndex] = Buffer[BufIndex];
			BufIndex++;
			CmdIndex++;
		}
		else if (getval == true) {
			Val[ValIndex] = Buffer[BufIndex];
			BufIndex++;
			ValIndex++;
		}
	}

	file.close();
	return 0;
}

int main() {
	int run = 1;

	Drive* drive = new Drive;
	Ext* ext = new Ext(drive);

	/* Print program name and copyright info */
	cout << "Disk Image Recovery Tool (DIRT) - Version " << VersionHigh << "." << VersionMid << "." << VersionLow << "\n";
	cout << "   Copyright 2014 Marcus Kelly\n";

	LoadConfig(drive, ext);

	cout << "\nType h for help\n";

	/* Main loop */
	while(run) {
		string cmd;
		cout << "\n- ";
		cin >> cmd;

		/* Add drive command */
		if (cmd.compare(0, 3, "add") == 0) {
			if (cmd.length() < 4) {
				cout << "Invalid input for command a\n";
				continue;
			}
			cout << "Adding " << cmd.substr(4, cmd.length()-4) << "\n";
			drive->AddDevice(cmd.substr(4, cmd.length()-4));
		}
		/* List drives command */
		else if (cmd.compare(0, 4, "list") == 0) {
			drive->ShowDevices();
		}

		else if (cmd.compare(0, 6, "delete") == 0) {
			string Device;
			cout << "Delete Drive\n";
			drive->ShowDevices();
			cout << "Select Drive:";
			cin >> Device;
			drive->DelDevice(atoi(Device.c_str()));
		}

		else if (cmd.compare(0, 4, "swap") == 0) {
			string Device1;
			string Device2;
			cout << "Swap Drives\n";
			drive->ShowDevices();
			cout << "Select Drive 1:";
			cin >> Device1;
			cout << "Select Drive 2:";
			cin >> Device2;
			drive->SwapDevices(atoi(Device1.c_str()), atoi(Device2.c_str()));
		}

		/* Set Block Size Command*/
		else if (cmd.compare(0, 2, "bs") == 0) {
			string opt;
			cout << "1:  512\n";
			cout << "2: 1024\n";
			cout << "3: 2048\n";
			cout << "4: 4096\n";
			cout << "5: 8192\n\n";
			cout << ":";
			cin >> opt;
			if (opt.compare(0, 1, "1") == 0) {
				drive->SetBufferSize(512);
			}
			if (opt.compare(0, 1, "2") == 0) {
				drive->SetBufferSize(1024);
			}
			if (opt.compare(0, 1, "3") == 0) {
				drive->SetBufferSize(2048);
			}
			if (opt.compare(0, 1, "4") == 0) {
				drive->SetBufferSize(4096);
			}
			if (opt.compare(0, 1, "5") == 0) {
				drive->SetBufferSize(8192);
			}
		}

		/* Stat Command */
		else if (cmd.compare(0, 4, "stat") == 0) {
			cout << "---------------------------------------\n";
			cout << "Block Size: " << drive->GetBufferSize() << "\n";
			cout << "Raid Level: " << drive->GetRaidLevel() << "\n";
			cout << "Raid Start Block: " << drive->GetRaidStartBlock() << "\n";
			cout << "Raid Block Size: " << drive->GetRaidBlockSize() << "\n";
			cout << "Raid Block Adjust: " << drive->GetRaidBlockAdjust() << "\n";
			cout << "File System: " << fileSystem << "\n";
			cout << "Current Directory: " << ext->GetCurrentDirectory() << "\n";
			cout << "---------------------------------------\n";
		}
		/* Set Drive Display All Command*/
		else if (cmd.compare(0, 3, "dda") == 0) {
			string opt;
			cout << "0: OFF\n";
			cout << "1: ON\n";
			cout << ":";
			cin >> opt;
      			if (opt.compare(0, 1, "0") == 0) {
				DrivesDisplayAll = false;
			}
      			if (opt.compare(0, 1, "1") == 0) {
				DrivesDisplayAll = true;
			}
		}
		/* Set Raid Block Size Command*/
		else if (cmd.compare(0, 3, "rbs") == 0) {
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
				drive->SetRaidBlockSize(16384);
			}
			if (opt.compare(0, 1, "2") == 0) {
				drive->SetRaidBlockSize(32768);
			}
			if (opt.compare(0, 1, "3") == 0) {
				drive->SetRaidBlockSize(65536);
			}
			if (opt.compare(0, 1, "4") == 0) {
				drive->SetRaidBlockSize(131072);
			}
			if (opt.compare(0, 1, "5") == 0) {
				drive->SetRaidBlockSize(262144);
			}
			if (opt.compare(0, 1, "6") == 0) {
				drive->SetRaidBlockSize(524288);
			}

		}
		/* Set Raid Start Block Command */
		else if (cmd.compare(0, 3, "rsb") == 0) {
			if (cmd.length() < 4) {
				cout << "Invalid input for command rsb\n";
				continue;
			}
			drive->SetRaidStartBlock(atoi(cmd.substr(4, cmd.length()-4).c_str()));
		}
		/* Set Current Directory Sector Command */
		else if (cmd.compare(0, 2, "sd") == 0) {
			if (cmd.length() < 3) {
				cout << "Invalid input for command sd\n";
				continue;
			}
			ext->SetCurrentDirectory(atoi(cmd.substr(3, cmd.length()-3).c_str()));
		}
		/* Change Directory Command */
    		else if (cmd.compare(0, 2, "cd") == 0) {
			if (cmd.length() < 2) {
				cout << "Invalid input for command cd\n";
				continue;
        		}
		        string Directory = cmd.substr(3, cmd.length()-3);
			ext->ChangeDirectory(Directory);
		}
		/* Change Directory Command */
    		else if (cmd.compare(0, 2, "fd") == 0) {
			if (cmd.length() < 2) {
				cout << "Invalid input for command cd\n";
				continue;
        		}
		        string Directory = cmd.substr(3, cmd.length()-3);
			ext->FileDump(Directory);
		}

		/* Show Directory Command */
		else if (cmd.compare(0, 2, "ls") == 0 || cmd.compare(0, 3, "dir") == 0) {
			ext->ShowDirectory();
		}


		/* Set Raid Level Command */
		else if (cmd.compare(0, 2, "rl") == 0) {
			if (cmd.length() < 3) {
				cout << "Invalid input for command a\n";
				continue;
			}
			drive->SetRaidLevel(atoi(cmd.substr(3, cmd.length()-3).c_str()));
    		}

		/* Set Raid Block Adjust Command */
		else if (cmd.compare(0, 2, "ba") == 0) {
			if (cmd.length() < 3) {
				cout << "Invalid input for command a\n";
				continue;
			}
			drive->SetRaidBlockAdjust(atoi(cmd.substr(3, cmd.length()-3).c_str()));
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

			char * data = drive->Read(dumpIndex);

			if (DrivesDisplayAll == 1) {
				for (int j = 0; j < drive->GetDeviceCount(); j++) {
					for (int i = 0; i < drive->GetBufferSize(); i++) {
						data = drive->GetAltDeviceData(j);
						if (cmd.compare(0, 1, "d") == 0) {
							if (data[i] >= 0x20 && data[i] <= 0x7E) {
								cout << data[i];
							}
							else {
								cout << ".";
		            				}
						}
						else if (cmd.compare(0, 1, "u") == 0) {
							cout << uchar2hex(data[i]) << " ";
						}
					}
					cout << "\n\n";
				}
			}
			else {
				for (int i = 0; i < drive->GetBufferSize(); i++) {
					if (cmd.compare(0, 1, "d") == 0) {
						if (data[i] >= 0x20 && data[i] <= 0x7E) {
							cout << data[i];
						}
						else {
							cout << ".";
		            			}
					}
					else if (cmd.compare(0, 1, "u") == 0) {
						cout << uchar2hex(data[i]) << " ";
					}
				}
				cout << "\n\n";
			}

			//dumpIndex++;
			dumpIndex += drive->GetSectorsPerBuffer();
      
		}
		else if (cmd.compare(0, 4, "pull") == 0) {
			if (cmd.length() > 5) {
				filename = cmd.substr(5, cmd.length()-5);
				inode = atoi(cmd.substr(5, cmd.length()-5).c_str());
			}
			ext->PullInode(inode, filename);
		}
	    	else if (cmd.compare(0, 5, "inode") == 0) {
			if (cmd.length() > 6) {
				inode = atoi(cmd.substr(6, cmd.length()-1).c_str());
			}

			cout << "INODE " << inode << "\n";

			ext_inode myinode = ext->ReadInode(inode);

			cout << "i_mode: " << myinode.i_mode << "\n";
			cout << "i_uid: " << myinode.i_uid << "\n";
			cout << "i_size_lo: " << myinode.i_size_lo << ", i_size_high: " << myinode.i_size_high << "\n";
			cout << "i_atime: " << myinode.i_atime << ", i_atime_extra: " << myinode.i_atime_extra << "\n";
			cout << "i_ctime: " << myinode.i_ctime << ", i_ctime_extra: " << myinode.i_ctime_extra << "\n";
			cout << "i_mtime: " << myinode.i_mtime << ", i_mtime_extra: " << myinode.i_mtime_extra << "\n";
			cout << "i_dtime: " << myinode.i_dtime << "\n";
			cout << "i_gid: " << myinode.i_gid << "\n";
			cout << "i_links_count: " << myinode.i_links_count << "\n";
			cout << "i_blocks_lo: " << myinode.i_blocks_lo << "\n";
			cout << "i_flags: " << myinode.i_flags << "\n";
			cout << "osd1: " << uchar2hex(myinode.osd1[0]) << uchar2hex(myinode.osd1[1]) << uchar2hex(myinode.osd1[2]) << uchar2hex(myinode.osd1[3]) << "\n";
			cout << "i_block: " << uchar2hex(myinode.i_block[0]) << uchar2hex(myinode.i_block[1]) << uchar2hex(myinode.i_block[2]);
			cout << uchar2hex(myinode.i_block[3]) << uchar2hex(myinode.i_block[4]) << uchar2hex(myinode.i_block[5]);
			cout << uchar2hex(myinode.i_block[6]) << uchar2hex(myinode.i_block[7]) << uchar2hex(myinode.i_block[8]);
			cout << uchar2hex(myinode.i_block[9]);
		        cout << uchar2hex(myinode.i_block[10]) << uchar2hex(myinode.i_block[11]) << uchar2hex(myinode.i_block[12]);
			cout << uchar2hex(myinode.i_block[13]) << uchar2hex(myinode.i_block[14]) << uchar2hex(myinode.i_block[15]);
			cout << uchar2hex(myinode.i_block[16]) << uchar2hex(myinode.i_block[17]) << uchar2hex(myinode.i_block[18]);
			cout << uchar2hex(myinode.i_block[19]);
		        cout << uchar2hex(myinode.i_block[20]) << uchar2hex(myinode.i_block[21]) << uchar2hex(myinode.i_block[22]);
			cout << uchar2hex(myinode.i_block[23]) << uchar2hex(myinode.i_block[24]) << uchar2hex(myinode.i_block[25]);
			cout << uchar2hex(myinode.i_block[26]) << uchar2hex(myinode.i_block[27]) << uchar2hex(myinode.i_block[28]);
			cout << uchar2hex(myinode.i_block[29]);
		        cout << uchar2hex(myinode.i_block[30]) << uchar2hex(myinode.i_block[31]) << uchar2hex(myinode.i_block[32]);
			cout << uchar2hex(myinode.i_block[33]) << uchar2hex(myinode.i_block[34]) << uchar2hex(myinode.i_block[35]);
			cout << uchar2hex(myinode.i_block[36]) << uchar2hex(myinode.i_block[37]) << uchar2hex(myinode.i_block[38]);
			cout << uchar2hex(myinode.i_block[39]);
		        cout << uchar2hex(myinode.i_block[40]) << uchar2hex(myinode.i_block[41]) << uchar2hex(myinode.i_block[42]);
			cout << uchar2hex(myinode.i_block[43]) << uchar2hex(myinode.i_block[44]) << uchar2hex(myinode.i_block[45]);
			cout << uchar2hex(myinode.i_block[46]) << uchar2hex(myinode.i_block[47]) << uchar2hex(myinode.i_block[48]);
			cout << uchar2hex(myinode.i_block[49]);
		        cout << uchar2hex(myinode.i_block[50]) << uchar2hex(myinode.i_block[51]) << uchar2hex(myinode.i_block[52]);
			cout << uchar2hex(myinode.i_block[53]) << uchar2hex(myinode.i_block[54]) << uchar2hex(myinode.i_block[55]);
			cout << uchar2hex(myinode.i_block[56]) << uchar2hex(myinode.i_block[57]) << uchar2hex(myinode.i_block[58]);
			cout << uchar2hex(myinode.i_block[59]) << "\n";

			int pos = 0;
			ext_extent_header myeh;
			ext_extent_idx myei;
			ext_extent_leaf myee;
			memcpy(&myeh, &myinode.i_block[pos], 12);
			if (myeh.eh_magic == 0xF30A) {
				pos+=12;
				if (myeh.eh_depth == 1) {
					memcpy(&myei, &myinode.i_block[pos], 12);
					cout << "  ei_block: " << myei.ei_block << " ei_leaf_lo: " << myei.ei_leaf_lo << " ei_leaf_hi: " << myei.ei_leaf_hi << " ei_unused: " << myei.ei_unused << "\n";
					cout << "  Address: " << (myei.ei_leaf_lo*8) << "\n";
				}
				else {
					memcpy(&myee, &myinode.i_block[pos], 12);
					cout << "  ee_block: " << myee.ee_block << " ee_len: " << myee.ee_len << " ee_start_hi: " << myee.ee_start_hi << " ee_start_lo: " << myee.ee_start_lo << "\n";
					cout << "  Address: " << (myee.ee_start_lo*8) << "\n";
				}
			}
	
			cout << "i_generation: " << myinode.i_generation << "\n";
			cout << "i_file_acl_lo: " << myinode.i_file_acl_lo << "\n";
			cout << "i_obso_faddr: " << myinode.i_obso_faddr << "\n";
			cout << "osd2: " << uchar2hex(myinode.osd2[0]) << uchar2hex(myinode.osd2[1]) << uchar2hex(myinode.osd2[2]);
			cout << uchar2hex(myinode.osd2[3]) << uchar2hex(myinode.osd2[4]) << uchar2hex(myinode.osd2[5]);
			cout << uchar2hex(myinode.osd2[6]) << uchar2hex(myinode.osd2[7]) << uchar2hex(myinode.osd2[8]);
			cout << uchar2hex(myinode.osd2[9]) << uchar2hex(myinode.osd2[10]) << uchar2hex(myinode.osd2[11]) << "\n";
			cout << "i_extra_isize: " << myinode.i_extra_isize << "\n";
			cout << "i_checksum_hi: " << myinode.i_checksum_hi << "\n";
			cout << "i_crtime: " << myinode.i_crtime << ", i_crtime_extra: " << myinode.i_crtime_extra << "\n";
			cout << "i_version_hi: " << myinode.i_version_hi << "\n";
	
			inode++;

		}
		/* Help command */
		else if (cmd.compare("h") == 0) {
			help();
		}
		/* Search Start command */
		else if (cmd.compare(0, 2, "ss") == 0) {
			if (cmd.length() > 3) {
				searchIndex = atoi(cmd.substr(3, cmd.length()-1).c_str());
				searchIndexFlag = 1;
			}
		}

		/* Seach disk command */
		else if (cmd.compare(0, 1, "s") == 0) {
			// open search.txt
			ofstream file("search.txt", ios::trunc);
			if(!file)
				throw(runtime_error(strerror(errno)));
			int activeSearch = 1;
			string searchString = cmd.substr(2, cmd.length()-1);
			int stringIndex = 0;
			int bufferCount = 0;
			int bytesPerSec = 0;
			char * data;

			if (searchIndexFlag == 0) {
				searchIndex = 0;
			}
			searchIndexFlag = 0;

			// allow spaces in search with ^
			for (int s = 0; s < searchString.length(); s++) {
				if (searchString[s] == '^') {
					searchString[s] = ' ';
				}
			}

			cout << "Searching for " << searchString << "\n";
			file << "Searching for " << searchString << "\n";
			file.close();
			//cout << "Press any key to stop the search\n";

			struct timeval tv;
			setTimer(tv,5); //set up a delay timer

			streampos SectorCount = (drive->GetMaxDeviceSize()/512);

			while(activeSearch) {

				if (checkTimer(tv,1)==1) {
					bytesPerSec = bufferCount*drive->GetBufferSize();
					bufferCount = 0;
				}

				drive->Read(searchIndex);

				cout << "Block " << searchIndex << "/" << SectorCount << " [" << bytesPerSec << "B/Sec]   \r" << std::flush;
				bufferCount += drive->GetDeviceCount()+1;

				// search each buffer
				for(int i = 0; i < drive->GetDeviceCount(); i++) {
					for(int j = 0; j < drive->GetBufferSize(); j++) {
						data = drive->GetAltDeviceData(i);
						if (data[j] == searchString[stringIndex]) {
							if (stringIndex == searchString.length()) {
								ofstream file("search.txt", ios::out | ios::app | ios::binary);
								if(!file)
									throw(runtime_error(strerror(errno)));
								//file.seekp(0, ios::end);
								cout << "Found: Disk=" << drive->GetDevice(i) << " Block=" << searchIndex << "                    \n";
								file << "Found: Disk=" << drive->GetDevice(i) << " Block=" << searchIndex << "\n";
								file.close();
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
				searchIndex += drive->GetSectorsPerBuffer();
			} // while(activeSearch)
			file.close();
		}
		/* Reset configuration command */
		else if (cmd.compare("reset") == 0) {
			//ofstream file("dirt.cnf", ios::trunc);
			for (int i = 0; i < drive->GetDeviceCount(); i++) {
				drive->DelDevice(i);
			}
			drive->SetRaidLevel(1);
			drive->SetRaidBlockAdjust(0);
			drive->SetRaidBlockSize(65536);
			DrivesDisplayAll = true;
			
		}
		
		/* Quit and save configuration command */
		else if (cmd.compare("w") == 0) {
			ofstream file("dirt.cnf", ios::trunc);
			if(!file) {
				cout << "\nCould not open config file.\n";
				return 1;
			}
			for (int i = 0; i < drive->GetDeviceCount(); i++) {
				file << "Drive:" << drive->GetDevice(i) << ";\n";
			}
			file << "RaidLevel: " << drive->GetRaidLevel() << ";\n";
			file << "RaidBlockAdjust:" << drive->GetRaidBlockAdjust() << ";\n";
			file << "RaidBlockSize:" << drive->GetRaidBlockSize() << ";\n";
			if (DrivesDisplayAll == false) {
				file << "DrivesDisplayAll:0;\n";
			}
			else {
				file << "DrivesDisplayAll:1;\n";
			}
			file << "Root:" << ext->GetCurrentDirectory() << ";\n";
			run = 0;
		}
		/* Quit command */
		else if (cmd.compare("q") == 0) {
			run = 0;
		}
	}
}

