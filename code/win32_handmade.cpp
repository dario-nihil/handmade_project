#include <windows.h>
#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static 

//typedef unsigned char uint8;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// TODO this is global for now
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
// global_variable HBITMAP BitmapHandle;
// global_variable HDC BitmapDeviceContext;

//DIB stands for Define Indipendent Bitmap, and represent things that can be represented as bitmap by the windows GDI
internal void Win32ResizeDIBSection(int Width, int Height)
{
  // TODO Bulletproof this.
  // Maybe don't free first, free after, then free first if that fails.

  // if(BitmapHandle)
  // {
  //   DeleteObject(BitmapHandle);
  // }

  // if(!BitmapDeviceContext)
  // {
  //   // TODO Should we recreare these under certain special circumstances
  //   BitmapDeviceContext = CreateCompatibleDC(0);
  // }

  if(BitmapMemory)
  {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;  // negativw value means DIB origin is upper.left corner, and bitmap is top-down 
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  // Note: Thank you to Chris Hecker of Spy Party Fame 
  // for clarifying the deal with StretchDIBits and BitBlt!
  // No mre DC for us.

  // 4 bytes
  int BytesPerPixel = 4;

  // To manually allocate memory you can use HeapAlloc or VirtualAlloc
  int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // difference between two consecutive rows (to move to the next row)
  int Pitch = Width * BytesPerPixel;

  uint8 *Row = (uint8 *)BitmapMemory;

  for(int Y = 0; Y < BitmapHeight; ++Y) // Rows
  {
    uint8 *Pixel = (uint8 *)Row;
    for(int X = 0; X < BitmapWidth; ++X) // Pixels in Row
    {
      /*
        Pixel in memory 00 00 00 00
      */

      *Pixel = (uint8)X;
      ++Pixel;

      *Pixel = (uint8)Y;
      ++Pixel;

      *Pixel = 0;
      ++Pixel;

      *Pixel = 0;
      ++Pixel;
    }

    Row += Pitch;
  }
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
  int WindowWidth = WindowRect->right - WindowRect->left;
  int WindowHeight = WindowRect->bottom - WindowRect->top;

  // StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height, 
  //   BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, 
    BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  //CreateDIBSection();
}

LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message)
  {
    case WM_SIZE: 
    {
      OutputDebugStringA("WM_SIZE\n");
      // Create Buffer to draw in
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int width = ClientRect.right - ClientRect.left;
      int height = ClientRect.bottom - ClientRect.top;

      Win32ResizeDIBSection(width, height);
      OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY:
    {
      // TODO handle this as an error -  recreate window?
      Running = false;
      OutputDebugStringA("WM_DESTROY\n");
    } break;
    case WM_CLOSE:
    {
      // TODO handle this with a message to the user?
      Running = false;
      OutputDebugStringA("WM_CLOSE\n");
    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    case WM_PAINT: 
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      long X = Paint.rcPaint.left;
      long Y = Paint.rcPaint.top;
      long Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      long Width = Paint.rcPaint.right - Paint.rcPaint.left;

      RECT ClientRect;
      GetClientRect(Window, &ClientRect);

      Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
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

  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallBack;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
    
    

    if(RegisterClass(&WindowClass))
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
        Running = true;
        while(Running)
        {
          MSG Message;
          BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
          if(MessageResult > 0)
          {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
          }
          else
          {
            break;
          }
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