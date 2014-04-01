/**********************************************
 * Disk Image Recovery Tool (DIRT)
 * -------------------------------------------
 * Created By        Marcus Kelly
 *    Contact        nexuslite@gmail.com
 * Created On        Feb 23, 2014
 * Updated On        Mar 31, 2014
 * -------------------------------------------
 * Copyright 2014 Marcus Kelly
 **********************************************/
#ifndef DRIVE
#define DRIVE

#include <iostream>
#include <vector>
#include <fstream>

#define DRIVE_BUF_SIZE 512
#define DRIVE_MAX_DEVICES 3
#define DRIVE_MAX_SEEK 2147483648

using namespace std;

class Drive {
	public:
		Drive() {
			DeviceCount = 0;
			IgnoredDrive = -1;
			RaidLevel = 1;
			RaidStartBlock = 0;
			RaidBlockAdjust = 0;
			RaidBlockSize = 65536;
			BufferSize = DRIVE_BUF_SIZE;
			SetDriveMax(DRIVE_MAX_DEVICES);
			// Read Raid Headers
		}
		~Drive() { }
		void SetRaidStartBlock(int InRaidStartBlock) {
			RaidStartBlock = InRaidStartBlock;
		}
		unsigned int GetRaidStartBlock() {
			return RaidStartBlock;
		}
		void SetRaidBlockSize(int InRaidBlockSize) {
			RaidBlockSize = InRaidBlockSize;
		}
		int GetRaidBlockSize() {
			return RaidBlockSize;
		}
		void SetRaidBlockAdjust(int InRaidBlockAdjust) {
			RaidBlockAdjust = InRaidBlockAdjust;
		}
		unsigned int GetRaidBlockAdjust() {
			return RaidBlockAdjust;
		}
		int SetRaidLevel(int InRaidLevel) {
			if (InRaidLevel > 6) {
				return 1;
			}
			RaidLevel = InRaidLevel;
			return 0;
		}
		int GetRaidLevel() {
			return RaidLevel;
		}
		void SetBufferSize(int InBufferSize) {
			BufferSize = InBufferSize;
			Buffer.resize(DriveMax);
			for(int i = 0; i < DriveMax; i++) {
				Buffer[i].resize(BufferSize);
			}
		}
		int GetBufferSize() {
			return BufferSize;
		}
		void SetDriveMax(int InDriveMax) {
			DriveMax = InDriveMax;
			Device.resize(DriveMax);
			Buffer.resize(DriveMax);
			for(int i = 0; i < DriveMax; i++) {
				Buffer[i].resize(BufferSize);
			}
		}
		int GetDriveMax() {
			return DriveMax;
		}
		int SwapDevices(int InDevice1, int InDevice2) {
			if (InDevice1 >= DeviceCount || InDevice2 >= DeviceCount) {
				return 1;
			}
			string First = Device[InDevice1];
			Device[InDevice1] = Device[InDevice2];
			Device[InDevice2] = First;
			return 0;
		}
		int AddDevice(string InDevice) {
			if (DeviceCount >= DriveMax) {
				return 1;
			}
			Device[DeviceCount] = InDevice;
			DeviceCount++;
			return 0;
		}
		int DelDevice(int InDevice) {
			if (InDevice >= DeviceCount) {
				return 1;
			}
			DeviceCount--;
			for (int i = InDevice; i < DeviceCount; i++) {
				Device[i] = Device[i+1];
			}
			Device[DeviceCount] = "";
			return 0;			
		}
		int GetSectorsPerBuffer() {
			return BufferSize/512;
		}
		char * Read(unsigned int Sector) {
			unsigned int BlockNumber, DriveBlock, SectorsPerBlock, DataDrive;
			//cout << "Raid Level: " << RaidLevel << "\n";
			switch (RaidLevel) {
				case 5: 
					SectorsPerBlock = RaidBlockSize/512;
					BlockNumber = (Sector/SectorsPerBlock)+RaidBlockAdjust;
			        	DriveBlock = Sector%(SectorsPerBlock)+(BlockNumber/(DeviceCount-1)*SectorsPerBlock)+RaidStartBlock;
					DataDrive = BlockNumber%(DeviceCount);
					break;
				case 1:
					DriveBlock = Sector;
					DataDrive = 0;
					break;
			}

		        ZeroIgnoredDrive();
		        ReadDrives(DriveBlock);
		        CreateIgnoredDrive();

			return &Buffer[DataDrive][0];

		}
		char * GetAltDeviceData(int InDevice) {
			if (InDevice > DeviceCount) {
				return 0;
			}
			return &Buffer[InDevice][0];
		}
		int GetDeviceCount() {
			return DeviceCount;
		}
		string GetDevice(int InDevice) {
			return Device[InDevice];
		}
		streampos GetMaxDeviceSize() {
			streampos Max = 0;
			for(int i = 0; i < DeviceCount; i++) {

				string diskError = string() + Device[i] + ": ";
				ifstream disk(Device[i].c_str(), ios_base::binary);
				if(!disk) {
					return 0;
				}

			        dsize = disk.tellg();
			        disk.seekg( 0, ios::end );
			        dsize = disk.tellg();
				if (dsize < Max || Max == 0) {
					Max = (unsigned long long) dsize;
				}

				disk.close();
			}
			return Max;
		}
		void ShowDevices() {
			for (int i = 0; i < DeviceCount; i++) {
				if (Device[i].compare("") == 0) {
					cout << i << " [missing]\n";
				}
				else {
					cout << i << " " << Device[i] << "\n";
				}
			}
		}
	protected:
		void ZeroIgnoredDrive() {
			for(int i = 0; i < DeviceCount; i++) {
				if (Device[i].compare("") == 0) {
					IgnoredDrive = i;
					for (int c = 0; c < BufferSize; c++) {
						Buffer[IgnoredDrive][c] = 0;
					}
					continue;
				}
			}
		}
		int ReadDrives(unsigned int Index) {
			for(int i = 0; i < DeviceCount; i++) {
				if (IgnoredDrive == i) {
					continue;
        			}
				string diskError = string() + Device[i] + ": ";
				ifstream disk(Device[i].c_str(), ios_base::binary);
				if(!disk) {
					return 1;
				}

			        dsize = disk.tellg();
			        disk.seekg( 0, ios::end );
			        dsize = disk.tellg() - dsize;
				dsize = dsize/512;

				dsize -= 512;

				if (Index >= dsize) { /* this needs fixed */
					return 1;
				}

				int IndexCopy = Index;
				disk.seekg(0, ios::beg);
				while(IndexCopy >= DRIVE_MAX_SEEK/512) {
					disk.seekg(DRIVE_MAX_SEEK, ios::cur);
					IndexCopy -= DRIVE_MAX_SEEK/512;
				}
				disk.seekg(IndexCopy*512, ios::cur);

				if(!disk) {
					//cout << Index << "\n";
					return 1; //throw(runtime_error(diskError + std::strerror(errno)));
				}
				disk.read(&Buffer[i][0], BufferSize);
				if(!disk) {
					//cout << Index << "\n";
					return 1; //throw(runtime_error(diskError + std::strerror(errno)));
				}

				disk.close();
			}
			return 0;
		}
		void CreateIgnoredDrive() {
			if (IgnoredDrive >= 0) {
				for (int j = 0; j < DeviceCount; j++) {
					if (IgnoredDrive == j) {
						continue;
					}
					for(int i = 0; i < BufferSize; i++) {
						Buffer[IgnoredDrive][i] ^= Buffer[j][i];
					}
				}
			}
		}
		vector <string> Device;
		int RaidLevel;
		int RaidBlockSize;
		unsigned int RaidStartBlock;
		unsigned int RaidBlockAdjust;
		int DeviceCount;
		int DriveMax;
		int IgnoredDrive;
		int BufferSize;
		string Error;
		streampos dsize;
		vector<vector <char> > Buffer;
};

#endif
