#include "hidmanager.h"
#include "hiddevice.h"
#include "commandparser.h"
#include <stdio.h>   
#include <stdlib.h>
#include <io.h>
#include <vector>

extern "C"
{
#include "builtin.h"
}

/* Members */
static int waitTime = 0;
static std::vector<std::wstring> commands;

static int readReport(HANDLE& handle, HidDevice hid_device) {
	if (handle != 0 && handle != INVALID_HANDLE_VALUE)
	{
		HIDP_CAPS caps = hid_device.get_device_capabilities(handle);

		const size_t outputBufferSize = caps.OutputReportByteLength;
		char* outputBuffer = new char[outputBufferSize];

		DWORD dwRead = 0;
		BOOL res;
		OVERLAPPED overlap = {};
		overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		HANDLE ev = overlap.hEvent;

		if (!ReadFile(handle, outputBuffer, outputBufferSize, &dwRead, &overlap)) {
			if (GetLastError() != ERROR_IO_PENDING) {
				std::cout << "not io pending" << std::endl;
				CloseHandle(ev);
				return 0;
			}
		}

		if (waitTime > 0) {		
			res = WaitForSingleObject(ev, waitTime * 1000);
			if (res != WAIT_OBJECT_0) {
				CloseHandle(ev);
				return 0;
			}
		}

		res = GetOverlappedResult(handle, &overlap, &dwRead, TRUE);
		if (res && dwRead > 0) {
			//get overlapped data
			char* data = new char[outputBufferSize];
			memcpy(data, &outputBuffer[1], (size_t)(outputBufferSize > dwRead ? dwRead : outputBufferSize));
			std::cout << data;
		}

		CloseHandle(ev);
	}

	return 0;
}

static int setreport_execution_block(std::wstring device, bool* foundDevice)
{
	*foundDevice = true;
	HidDevice hid_device(device);
	HANDLE handle = hid_device.get_device_handle(device);
	HIDP_CAPS caps = hid_device.get_device_capabilities(handle);

	if (handle != 0 && handle != INVALID_HANDLE_VALUE)
	{
		for (std::wstring command : commands) {
			const size_t reportDataSize = command.size() + 1; //+1 - termination character (\0)
			const size_t sendingReportSize = caps.OutputReportByteLength;

			if (reportDataSize > sendingReportSize) {
				std::cout << "warn: report larger than maximum size" << std::endl;
			}

			//prepare report data	
			char* reportData = new char[reportDataSize];

			// conversion
			size_t i;
			wcstombs_s(&i, reportData, reportDataSize, command.c_str(), reportDataSize);

			//prepare report
			char* reportBuffer = new char[sendingReportSize];

			// set report id
			reportBuffer[0] = 0x01;

			// copy data into the report buffer
			strncpy_s(&reportBuffer[1], sendingReportSize, reportData, sendingReportSize);

			// send the report
			if (HidD_SetOutputReport(handle, reportBuffer, sendingReportSize)) {
				readReport(handle, hid_device); //read report
			} else {
				std::cout << "setreport: send error (code: " << GetLastError() << ")" << std::endl;
			}
		}

		CloseHandle(handle);
	}

	return 0;
}

int cmd_setreport(int argc, const char **argv)
{
	if (_isatty(_fileno(stdin))) { //stdin is a terminal
		//lookup -d
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-d") == 0)
			{
				for (int j = i + 1; j < argc; j++)
				{
					if (argv[j][0] == '-')
					{
						break;
					}

					//parse commands
					const char* str = argv[j];
					std::wstring data(str, str + strlen(str));
					if (data.length() % 2 == 0 && data.substr(0, 2) == L"0x") {
						commands.push_back(data.substr(2, data.length()));
					}
					else {
						commands.push_back(data);
					}
				}

				break;
			}
		}
	} else { //stdin is a file or a pipe
		std::string inputStr;
		std::cin >> inputStr;
		std::wstring input(inputStr.begin(), inputStr.end());
		if (!input.empty()) {
			commands.push_back(input);
		}
	}
	
	if (commands.size() == 0) {
		std::cout << 0 << std::endl;
		return 0;
	}
	
	std::wstring _waitTime = get_w_param(argc, argv, "-w");
	if (!_waitTime.empty()) {
		int wt = std::stoi(_waitTime);
		waitTime = wt > 0 ? wt : 0;
	}

	HidManager hid_manager;
	return hid_manager.process_devices(argc, argv, &setreport_execution_block);
}