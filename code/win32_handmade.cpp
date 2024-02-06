#include <windows.h>
#include <stdint.h>
#include <Xinput.h>

#define internal static 
#define local_persist static 
#define global_variable static 

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer {
//NOTE Pixels are always 32-bits wide, Memory Order BB GG RR XX little endian
 BITMAPINFO Info;
 void *Memory;
 int Width;
 int Height;
 int Pitch; // difference between two consecutive rows (to move to the next row)
};

// TODO this is global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension
{
  int Width;
  int Height;
};

// Note: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
  return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Note: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
  return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(void)
{
  HMODULE XinputLibrary = LoadLibraryA("xinput1_4.dll");
  if(XinputLibrary)
  {
    XInputGetState = (x_input_get_state *)GetProcAddress(XinputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XinputLibrary, "XInputSetState");
  }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return(Result);
}

// global_variable HBITMAP BitmapHandle;
// global_variable HDC BitmapDeviceContext;

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
  //TODO let's see what the optimizer does

  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; ++Y) // Rows
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = 0; X < Buffer->Width; ++X) // Pixels in Row
    {
      /*
        Pixel in memory 00 00 00 00
      */

     uint8 Blue = (X + XOffset);
     uint8 Green = (Y + YOffset);

     // 0x xx rr gg bb -> bb gg rr xx becaue the processor is little endian
      *Pixel++ = ((Green << 8) | Blue);
    }

    Row += Buffer->Pitch;
    // Row = (uint8 *)Pixel; is equivalent as the instruction above
  }
}

//DIB stands for Define Indipendent Bitmap, and represent things that can be represented as bitmap by the windows GDI
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,  int Width, int Height)
{
  // TODO Bulletproof this.
  // Maybe don't free first, free after, then free first if that fails.

  if(Buffer->Memory)
  {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  int BytesPerPixel = 4; // 4 bytes

  //NOTE When the bitHeight is negative, this is the clue to
  // Windows to treat this bitmap as top-down, not bottom-up, meaning that
  // the first three bytes of the image are the color for the top left pixel
  // in the bitmap, not the bottom left!
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;  // negativw value means DIB origin is upper.left corner, and bitmap is top-down 
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  // Note: Thank you to Chris Hecker of Spy Party Fame 
  // for clarifying the deal with StretchDIBits and BitBlt!
  // No mre DC for us.

  // To manually allocate memory you can use HeapAlloc or VirtualAlloc
  int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  Buffer->Pitch = Width * BytesPerPixel;

  //TODO Probably clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
  //TODO aspect ratio correction
  //TODO play with stretch mode
  StretchDIBits(DeviceContext, 
                0, 0, WindowWidth, WindowHeight, 
                0, 0, Buffer->Width, Buffer->Height,
                Buffer->Memory, &Buffer->Info, 
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message)
  {
    case WM_DESTROY:
    {
      // TODO handle this as an error -  recreate window?
      GlobalRunning = false;
      OutputDebugStringA("WM_DESTROY\n");
    } break;
    case WM_CLOSE:
    {
      // TODO handle this with a message to the user?
      GlobalRunning = false;
      OutputDebugStringA("WM_CLOSE\n");
    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_IME_KEYDOWN:
    case WM_KEYUP:
    {
      uint32 VKCode = WParam; // what key actually went down
      bool WasDown = ((LParam & (1 << 30)) != 0); // equivalent to ((LParam >> 30) & (1))
      bool IsDown = ((LParam & (1 << 31)) == 0);
      if(WasDown != IsDown)
      {
        if(VKCode == 'W')
        {
          OutputDebugStringA("W\n");
        }
        else if(VKCode == 'A')
        {
          OutputDebugStringA("A\n");
        }
        else if(VKCode == 'S')
        {
          OutputDebugStringA("S\n");
        }
        else if(VKCode == 'D')
        {
          OutputDebugStringA("D\n");
        }
        else if(VKCode == 'Q')
        {
          OutputDebugStringA("Q\n");
        }
        else if(VKCode == 'E')
        {
          OutputDebugStringA("E\n");
        }
        else if(VKCode == VK_UP)
        {
          OutputDebugStringA("UP\n");
        }
        else if(VKCode == VK_LEFT)
        {
          OutputDebugStringA("LEFT\n");
        }
        else if(VKCode == VK_DOWN)
        {
          OutputDebugStringA("DOWN\n");
        }
        else if(VKCode == VK_RIGHT)
        {
          OutputDebugStringA("RIGHTN\n");
        }
        else if(VKCode == VK_ESCAPE)
        {
          OutputDebugStringA("ESCAPE: ");
          if(IsDown)
          {
            OutputDebugStringA("IsDown ");
          }
          if(WasDown)
          {
            OutputDebugStringA("WasDown ");
          }
          OutputDebugStringA("\n");
        }
        else if(VKCode == VK_SPACE)
        {
          OutputDebugStringA("SPACE\n");
        }
      }

    } break;
    case WM_PAINT: 
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      long X = Paint.rcPaint.left;
      long Y = Paint.rcPaint.top;
      long Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      long Width = Paint.rcPaint.right - Paint.rcPaint.left;

      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
      local_persist DWORD Operation = WHITENESS; // this is needed???
      EndPaint(Window, &Paint);
    } break;
    default:
    {
      //OutputDebugStringA("DEFAULT\n");
      Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
  }

  return(Result);
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CommandLine,
  int       ShowCode
)
{
  Win32LoadXInput();
  WNDCLASSA WindowClass = {};

  // Create Buffer to draw in
  Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

  WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  WindowClass.lpfnWndProc = Win32MainWindowCallBack;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
    
    if(RegisterClassA(&WindowClass))
    {
      HWND WindowHandle = CreateWindowEx(
        0, 
        WindowClass.lpszClassName,
        "Handmade Hero", 
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT,
        0,
        0,
        Instance,
        0
      );

      if(WindowHandle) 
      {
        //NOTE Since we specified CS_OWNDC, wecan just
        // gt one device context and use it forever because we
        // are not sharing it with anyone
        int XOffset = 0;
        int YOffset = 0;

        GlobalRunning = true;
        while(GlobalRunning)
        {
          MSG Message;

          while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
          {
            if(Message.message == WM_QUIT)
            {
              GlobalRunning = false;
            }

            TranslateMessage(&Message);
            DispatchMessage(&Message);
          }

          //TODO Should we pull this more frequently
          for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++ )
          {
            XINPUT_STATE ControllerState;
            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
              // This controller is plugged in
              //TODO see if ControllerState.dwPacketNumber increment too rapidly
              XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

              bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
              bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
              bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
              bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
              bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
              bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
              bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
              bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
              bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
              bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
              bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
              bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

              int16 StickX = Pad->sThumbLX;
              int16 StickY = Pad->sThumbLY;

              if(AButton)
              {
                YOffset += 2;
              }
            }
            else
            {
              // this controller is not avaliable
            }
          }
          
          RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

          HDC DeviceContext = GetDC(WindowHandle);

          win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);

          Win32DisplayBufferInWindow(&GlobalBackbuffer,
                                      DeviceContext, 
                                      Dimension.Width, Dimension.Height);
          //ReleaseDC(WindowHandle, DeviceContext); because we use CS_OWNDC

          ++XOffset;
          //YOffset += 2;
        }
      }
      else
      {
        //TODO logging
      }
    }
    else
    { 
      // TODO logging
    }

    return (0);
}