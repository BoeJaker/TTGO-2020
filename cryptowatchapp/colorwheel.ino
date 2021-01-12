struct rgb {
  int red;
  int green;
  int blue;
};

// RGB to 3-byte HEX, Color Code Conversion
unsigned long createRGB(int r, int g, int b)
{   
    return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}

// RGBA to 4-byte HEX, Color Code Conversion
unsigned long createRGBA(int r, int g, int b, int a)
{   
    return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8) + (a & 0xff);
}
// Color wheel incrementer, fades through all colors
int wheel_pos;
void color_wheel()
{
  x=1; // placeholder
}
// Color 
int fade_pos;
void fade()
{
  x=1; // placeholder  
}

int seed;
void random_blend()
{
  x=1; // placeholder  
}
