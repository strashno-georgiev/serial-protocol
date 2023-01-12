using System;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Segger;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;

class Program : IWin32Window {
  [DllImport("kernel32.dll")]
  internal static extern IntPtr GetConsoleWindow();

  public IntPtr Handle {
    get {
      return GetConsoleWindow();
    }
  }
  /// <summary>
  /// Opens an instance to a USBBULK device and send 1 byte to the device and reads it back.
  /// </summary>
  /// <param name="deviceIndex">Device Index that shall be used.</param>
  static private void DoEcho(int deviceIndex) {
    uint NumRepeats;
    byte[] data = new byte[64];
    int    r;
    int    NumBytes = 1;

    for (int i = 0; i < data.Length; i++) {
      data[i] = (byte)i;
    }
    try {
      UsbBulkDevice pDevice = new UsbBulkDevice(deviceIndex);
      if (pDevice.IsOpened == false) {
        MessageBox.Show(new Program(), "Could not open device", "Error opening the device", MessageBoxButtons.OK, MessageBoxIcon.Error);
        return;
      }
      Console.Write("Vendor: {0}, Product: {1}\n", pDevice.VendorName, pDevice.ProductName);
      Console.Write("Enter the number of bytes to be send to the echo client: ");
      NumRepeats = Convert.ToUInt32(Console.ReadLine());
      for (int i = 0; i < NumRepeats; i++) {
        pDevice.Write(data, NumBytes);
        r = pDevice.Read(data, NumBytes);
        if (r != NumBytes) {
          MessageBox.Show(string.Format("Wrong number of data received from device, Expected {0}, Got {1}", data.Length, r), "USBBULK error", MessageBoxButtons.OK, MessageBoxIcon.Error);
          break;
        }
        Console.Write(".");
      }
      pDevice.Close();
      Console.WriteLine("\nAll data have been sent to the client.");
    } catch (Exception e) {
      Console.ForegroundColor = ConsoleColor.Red;
      Console.WriteLine("An exception occurred:\n{0}:\n  {1}", e.Message, e.InnerException);
      Console.ResetColor();
    }
  }

  /// <summary>
  /// Main method of the program.
  /// </summary>
  /// <param name="args">Command line parameter given by start of the program.</param>
  static void Main(string[] args) {
    int devIndex;
    List<int> DevList;

  Retry:
    Console.Clear();
    Console.WriteLine("Looking for Devices...");
    UsbBulkDevice.AddAllowedDeviceItem(0x8765, 0x1234);
    UsbBulkDevice.AddAllowedDeviceItem(0x8765, 0x1240);
    UsbBulkDevice.AddAllowedDeviceItem(0x8765, 0x1241);
    while (UsbBulkDevice.DeviceCount == 0) {
      System.Threading.Thread.Sleep(100);
    }
    Console.WriteLine("Found {0:D} devices", UsbBulkDevice.DeviceCount);
    DevList = UsbBulkDevice.GetAvailableDeviceList();
    foreach (int iItem in DevList) {
      UsbBulkDevice pDevice = new UsbBulkDevice(iItem);
      Console.Write("Found the following device {0}:\n  SerialNo:    {1}\n  VendorName:  {2}\n  ProductName: {3}\n", iItem, UsbBulkDevice.GetSerialNo(iItem), pDevice.VendorName, pDevice.ProductName);
      Console.Write("  VendorId:    0x{0:x4}\n  ProductId:   0x{1:x4}\n  InterfaceNo: 0x{2:x2}\n", pDevice.VendorId, pDevice.ProductId, pDevice.InterfaceNo);
    }
    Console.Write("To which device do you want to connect: ");
    try {
      String ReadLine;

      ReadLine = Console.ReadLine();
      if (ReadLine.ToLower()[0] == 'q') {
        goto End;
      }
      devIndex = Convert.ToInt32(ReadLine);
    } catch (Exception e) {
      if (MessageBox.Show(new Program(), e.Message + "Do you want to retry", "Error occurred", MessageBoxButtons.RetryCancel, MessageBoxIcon.Exclamation) == DialogResult.Retry) {
        goto Retry;
      } 
      goto End;
    }
    DoEcho(devIndex);
    if (MessageBox.Show(new Program(), "Do you want to retry", "Question", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes) {
      goto Retry;
    } 
    End: ;

  }

}
