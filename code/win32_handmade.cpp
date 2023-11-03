#include <windows.h>

#define internal static 
#define local_persist static 
#define global_variable static 

// TODO this is global for now
global_variable bool Running;

LRESULT CALLBACK MainWindowCallBack(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message)
  {
    case WM_SIZE: 
    {
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
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      local_persist DWORD Operation = WHITENESS;
      PatBlt(DeviceContext, X, Y, Width, Height, Operation);
      Operation = Operation == WHITENESS ? BLACKNESS : WHITENESS;
      // if(Operation == WHITENESS)
      // {
      //   Operation = BLACKNESS;
      // }
      // else
      // {
      //   Operation = WHITENESS;
      // }

      EndPaint(Window, &Paint);
    } break;
    default:
    {
      OutputDebugStringA("DEFAULT\n");
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
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if(RegisterClass(&WindowClass))
    {
      HWND WindowHandle = CreateWindowExA(
        0, WindowClass.lpszClassName,
        "Handmade Hero", 
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT, 
        CW_USEDEFAULT,
        0,
        0,
        Instance,
        0);

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