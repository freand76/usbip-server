#include <wx/wx.h>
#include <stdio.h>
#include <signal.h>

#include "UsbDevice.h"
#include "UsbIpServer.h"

class Test : public wxAppConsole {
    bool OnInit();
    int OnRun();
    int FilterEvent(wxEvent& event); 
};

wxIMPLEMENT_APP(Test);

bool Test::OnInit() {
    if (!wxAppConsole::OnInit()) {
	return false;
    }
    wxPrintf("Hello World\n");
    return true;
}

int Test::OnRun() {
    UsbIpServer test;
    if (test.Init()) {
	wxPrintf("Init OK\n");
    } else {
	return false;
    }

    UsbDevice dev(0x00fa, 0xc001, 0x1234, 0xff, 0, 0);
    test.AddDevice(&dev, "My First Virtual Device", "1-1", 2, 3, 1);

    bool res = test.StartServer();
    wxPrintf("Res = %d\n", int(res));

    wxSleep(10);
    
    test.StopServer();

    return false;
}

int Test::FilterEvent(wxEvent& event) {
    if (event.GetEventType() == wxEVT_KEY_DOWN) {
	wxKeyEvent& keyEvent = (wxKeyEvent&)event;
	wxPrintf("Key = %d\n", keyEvent.GetKeyCode());
    }
    
    return -1;
}
