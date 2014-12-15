#include<string>
#include "hidmanager.h"
#include "hiddevice.h"

extern "C"
{
    #include "builtin.h"
}

/* Methods */
static int info_execution_block(std::wstring device, bool* foundDevice)
{
    HidDevice hid_device(device);
    HANDLE handle = hid_device.get_device_handle(device);

    if (handle != 0 && handle != INVALID_HANDLE_VALUE)
    {
        HIDD_ATTRIBUTES attributes = hid_device.get_device_attributes(handle);
        HIDP_CAPS capabilities = hid_device.get_device_capabilities(handle);
        std::wstring manufacturer = hid_device.get_manufacturer_string(handle);
        std::wstring product = hid_device.get_product_string(handle);
        std::wstring serial = hid_device.get_serial_string(handle);

		//remove 0x
		std::string vendorID = hid_device.number_to_hex_string(attributes.VendorID);
		std::string productID = hid_device.number_to_hex_string(attributes.ProductID);
		vendorID = vendorID.find("0x") == 0 ? vendorID.substr(2, vendorID.length()) : vendorID;
		productID = productID.find("0x") == 0 ? productID.substr(2, productID.length()) : productID;

        std::cout  << "================================================================" << std::endl;
        // USB should be modified to be other types should they become supported
        std::wcout << hid_device.clean_string(product) << " via " << "USB" << std::endl;
        std::cout  << "================================================================" << std::endl;
		std::cout  << "VendorID: " << vendorID << std::endl;
		std::cout  << "ProductID: " << productID << std::endl;
        std::cout  << "VersionNumber: " << attributes.VersionNumber << std::endl;
        std::wcout << "Manufacturer: " << hid_device.clean_string(manufacturer) << std::endl;
        std::wcout << "Product: " << hid_device.clean_string(product) << std::endl;
        std::wcout << "SerialNumber: " << serial << std::endl;
        std::wcout << "LocationID: " << hid_device.get_device_path() << std::endl;
        std::cout  << "PrimaryUsagePage: " << hid_device.number_to_hex_string(capabilities.UsagePage) << std::endl;
        std::cout  << "PrimaryUsage: " << capabilities.Usage << std::endl;
        std::cout  << "MaxInputReportSize: " << capabilities.InputReportByteLength << std::endl;
        std::cout  << "MaxOutputReportSize: " << capabilities.OutputReportByteLength << std::endl;
        std::cout  << "MaxFeatureReportSize: " << capabilities.FeatureReportByteLength << std::endl;

        CloseHandle(handle);
    }

    return 0;
}

int cmd_info(int argc, const char **argv)
{
    HidManager hid_manager;
    return hid_manager.process_devices(argc, argv, &info_execution_block);
}