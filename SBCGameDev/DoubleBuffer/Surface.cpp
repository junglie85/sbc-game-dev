// Template, major revision 3, beta
// IGAD/NHTV - Jacco Bikker - 2006-2009

#define MALLOC64(x) _aligned_malloc(x, 64)
#define FREE64(x) _aligned_free(x)

#include "surface.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

void NotifyUser(char* s);

// -----------------------------------------------------------
// True-color surface class implementation
// -----------------------------------------------------------

Surface::Surface(int a_Width, int a_Height, Pixel* a_Buffer)
    : m_Width(a_Width)
    , m_Height(a_Height)
    , m_Buffer(a_Buffer)
    , m_Pitch(a_Width)

{
}

Surface::Surface(int a_Width, int a_Height)
    : m_Width(a_Width)
    , m_Height(a_Height)
    , m_Pitch(a_Width)

{
    m_Buffer = (Pixel*)malloc(a_Width * a_Height * sizeof(Pixel));
}

Surface::Surface(char* a_File, MyFiles* FileHandler)
    : m_Buffer(NULL)
    , m_Width(0)
    , m_Height(0)

{
    // use the name to load the image and set up Height and Width
    m_Buffer = (Pixel*)FileHandler->Load(a_File, &m_Width, &m_Height);

    if (m_Buffer == NULL) {
        printf("File %s cannot be loaded, check name and dir \n", a_File);
    } else {
        printf("Buffer loaded with %s image, size is %i,%i\n", a_File, m_Width, m_Height);
    }
    SetPitch(m_Width);
}

Surface::~Surface() { free(m_Buffer); }

void Surface::ClearBuffer(Pixel a_Color)
{
    // loop through and set to clear
    for (int i = 0; i < m_Width * m_Height; i++)
        m_Buffer[i] = a_Color;
}

void Surface::Resize(int a_Width, int a_Height, Surface* a_Orig)
{
    unsigned int newpitch = (a_Width + 16) & 0xffff0; // calculate the new pitch size

    Pixel* src = a_Orig->GetBuffer();
    Pixel* dst = m_Buffer; // set up source and destination for moves

    int u, v, owidth = a_Orig->GetWidth();
    int oheight = a_Orig->GetHeight();
    int destx   = (owidth << 10) / a_Width;
    int desty   = (oheight << 10) / a_Height;

    for (v = 0; v < a_Height; v++) {
        for (u = 0; u < a_Width; u++) {
            int su = u * destx, sv = v * desty;

            Pixel* s = src + (su >> 10) + (sv >> 10) * owidth;

            int ufrac = su & 1023, vfrac = sv & 1023;

            int w4   = (ufrac * vfrac) >> 12;
            int w3   = ((1023 - ufrac) * vfrac) >> 12;
            int w2   = (ufrac * (1023 - vfrac)) >> 12;
            int w1   = ((1023 - ufrac) * (1023 - vfrac)) >> 12;
            int x2   = ((su + destx) > ((owidth - 1) << 10)) ? 0 : 1;
            int y2   = ((sv + desty) > ((oheight - 1) << 10)) ? 0 : 1;
            Pixel p1 = *s, p2 = *(s + x2), p3 = *(s + owidth * y2), p4 = *(s + owidth * y2 + x2);
            unsigned int r
                = (((p1 & REDMASK) * w1 + (p2 & REDMASK) * w2 + (p3 & REDMASK) * w3 + (p4 & REDMASK) * w4) >> 8)
                & REDMASK;
            unsigned int g
                = (((p1 & GREENMASK) * w1 + (p2 & GREENMASK) * w2 + (p3 & GREENMASK) * w3 + (p4 & GREENMASK) * w4) >> 8)
                & GREENMASK;
            unsigned int b
                = (((p1 & BLUEMASK) * w1 + (p2 & BLUEMASK) * w2 + (p3 & BLUEMASK) * w3 + (p4 & BLUEMASK) * w4) >> 8)
                & BLUEMASK;
            *(dst + u + v * newpitch) = (Pixel)(r + g + b);
        }
    }
    // reset the members to new values
    m_Width  = a_Width;
    m_Height = a_Height;
    m_Pitch  = newpitch;
}

void Surface::Line(float x1, float y1, float x2, float y2, Pixel c)
{
    if ((x1 < 0) || (y1 < 0) || (x1 >= m_Width) || (y1 >= m_Height) || (x2 < 0) || (y2 < 0) || (x2 >= m_Width)
        || (y2 >= m_Height)) {
        return;
    }
    float b = x2 - x1;
    float h = y2 - y1;
    float l = fabsf(b);
    if (fabsf(h) > l) l = fabsf(h);
    int il   = (int)l;
    float dx = b / (float)l;
    float dy = h / (float)l;
    for (int i = 0; i <= il; i++) {
        *(m_Buffer + (int)x1 + (int)y1 * m_Pitch) = c;
        x1 += dx, y1 += dy;
    }
}

void Surface::Plot(int xpos, int ypos, Pixel p)
{
    if ((xpos >= 0) && (ypos >= 0) && (xpos < m_Width) && (ypos < m_Height)) { m_Buffer[xpos + (ypos * m_Pitch)] = p; }
}

void Surface::DrawBox(int xTL, int yTL, int xBR, int yBR, Pixel p)
{
    Line((float)xTL, (float)yTL, (float)xBR, (float)yBR, p);
    Line((float)xBR, (float)yTL, (float)xBR, (float)yBR, p);
    Line((float)xTL, (float)yBR, (float)xBR, (float)yBR, p);
    Line((float)xTL, (float)yTL, (float)xTL, (float)yBR, p);
}

void Surface::Bar(int x1, int y1, int x2, int y2, Pixel c)
{
    Pixel* a = x1 + y1 * m_Pitch + m_Buffer;
    for (int y = y1; y <= y2; y++) {
        for (int x = 0; x <= (x2 - x1); x++)
            a[x] = c;
        a += m_Pitch;
    }
}

void Surface::CopyTo(Surface* a_Dst, int a_X, int a_Y)
{
    Pixel* dst = a_Dst->GetBuffer();
    Pixel* src = m_Buffer;
    if ((src) && (dst)) {
        int srcwidth  = m_Width;
        int srcheight = m_Height;
        int srcpitch  = m_Pitch;
        int dstwidth  = a_Dst->GetWidth();
        int dstheight = a_Dst->GetHeight();
        int dstpitch  = a_Dst->GetPitch();
        if ((srcwidth + a_X) > dstwidth) srcwidth = dstwidth - a_X;
        if ((srcheight + a_Y) > dstheight) srcheight = dstheight - a_Y;
        if (a_X < 0) src -= a_X, srcwidth += a_X, a_X = 0;
        if (a_Y < 0) src -= a_Y * srcpitch, srcheight += a_Y, a_Y = 0;
        if ((srcwidth > 0) && (srcheight > 0)) {
            dst += a_X + dstpitch * a_Y;
            for (int y = 0; y < srcheight; y++) {
                memcpy(dst, src, srcwidth * 4);
                dst += dstpitch;
                src += srcpitch;
            }
        }
    }
}

void Surface::BlendCopyTo(Surface* a_Dst, int a_X, int a_Y)
{
    Pixel* dst = a_Dst->GetBuffer();
    Pixel* src = m_Buffer;
    if ((src) && (dst)) {
        int srcwidth  = m_Width;
        int srcheight = m_Height;
        int srcpitch  = m_Pitch;
        int dstwidth  = a_Dst->GetWidth();
        int dstheight = a_Dst->GetHeight();
        int dstpitch  = a_Dst->GetPitch();
        if ((srcwidth + a_X) > dstwidth) srcwidth = dstwidth - a_X;
        if ((srcheight + a_Y) > dstheight) srcheight = dstheight - a_Y;
        if (a_X < 0) src -= a_X, srcwidth += a_X, a_X = 0;
        if (a_Y < 0) src -= a_Y * srcpitch, srcheight += a_Y, a_Y = 0;
        if ((srcwidth > 0) && (srcheight > 0)) {
            dst += a_X + dstpitch * a_Y;
            for (int y = 0; y < srcheight; y++) {
                for (int x = 0; x < srcwidth; x++)
                    dst[x] = AddBlend(dst[x], src[x]);
                dst += dstpitch;
                src += srcpitch;
            }
        }
    }
}

void Surface::CopyAlphaPlot(Surface* a_Dst, int a_X, int a_Y, int a_Alpha)
{
    Pixel* dst = a_Dst->GetBuffer();
    Pixel* src = m_Buffer;
    Pixel p;
    if ((src) && (dst)) {
        int srcwidth  = m_Width;
        int srcheight = m_Height;
        int srcpitch  = m_Pitch;
        int dstwidth  = a_Dst->GetWidth();
        int dstheight = a_Dst->GetHeight();
        int dstpitch  = a_Dst->GetPitch();
        if ((srcwidth + a_X) > dstwidth) srcwidth = dstwidth - a_X;
        if ((srcheight + a_Y) > dstheight) srcheight = dstheight - a_Y;
        if (a_X < 0) src -= a_X, srcwidth += a_X, a_X = 0;
        if (a_Y < 0) src -= a_Y * srcpitch, srcheight += a_Y, a_Y = 0;
        if ((srcwidth > 0) && (srcheight > 0)) {
            dst += a_X + dstpitch * a_Y;
            for (int y = 0; y < srcheight; y++) {
                for (int x = 0; x < srcwidth; x++) {
                    if (src[x] & ALPHAMASK) dst[x] = src[x] + a_Alpha; // this will stop us drawing colours but alpha 0
                }
                dst += dstpitch;
                src += srcpitch;
            }
        }
    }
}

void Surface::CopyBox(Surface* Src, int Width, int Height, int srcPitch, int DestX, int DestY)
{
    Pixel* dst = GetBuffer();
    Pixel* src = Src->GetBuffer();

    if ((src) && (dst)) {
        int dstpitch = GetPitch();

        {
            dst += DestX + (dstpitch * DestY);
            for (int y = 0; y < Height; y++) {
                memcpy(dst, src + 8, Width * 4);
                dst += dstpitch;
                src += srcPitch;
            }
        }
    }
}

Sprite::Sprite(Surface* a_Surface, unsigned int a_NumFrames)
    : m_Width(a_Surface->GetWidth() / a_NumFrames)
    , m_Height(a_Surface->GetHeight())
    , m_Pitch(a_Surface->GetWidth())
    , m_NumFrames(a_NumFrames)
    , m_CurrentFrame(0)
    , m_Flags(0)
    , m_Start(new unsigned int*[a_NumFrames])
    , m_Surface(a_Surface)
{
    InitializeStartData();
}

Sprite::~Sprite()
{
    delete m_Surface;
    for (unsigned int i = 0; i < m_NumFrames; i++)
        delete m_Start[i];
    delete m_Start;
}

void Sprite::Draw(int a_X, int a_Y, Surface* a_Target)
{
    if ((a_X < -m_Width) || (a_X > (a_Target->GetWidth() + m_Width))) return;
    if ((a_Y < -m_Height) || (a_Y > (a_Target->GetHeight() + m_Height))) return;
    int x1 = a_X, x2 = a_X + m_Width;
    int y1 = a_Y, y2 = a_Y + m_Height;
    Pixel* src = GetBuffer() + m_CurrentFrame * m_Width;
    if (x1 < 0) {
        src += -x1;
        x1 = 0;
    }
    if (x2 > a_Target->GetWidth()) x2 = a_Target->GetWidth();
    if (y1 < 0) {
        src += -y1 * m_Pitch;
        y1 = 0;
    }
    if (y2 > a_Target->GetHeight()) y2 = a_Target->GetHeight();
    Pixel* dest = a_Target->GetBuffer();
    int xs;
    const int dpitch = a_Target->GetPitch();
    if ((x2 > x1) && (y2 > y1)) {
        unsigned int addr = y1 * dpitch + x1;
        const int width   = x2 - x1;
        const int height  = y2 - y1;
        for (int y = 0; y < height; y++) {
            const int line = y + (y1 - a_Y);
            const int lsx  = m_Start[m_CurrentFrame][line] + a_X;
            if (m_Flags & FLARE) {
                xs = (lsx > x1) ? lsx - x1 : 0;
                for (int x = xs; x < width; x++) {
                    const Pixel c1 = *(src + x);
                    if (c1) {
                        const Pixel c2     = *(dest + addr + x);
                        *(dest + addr + x) = AddBlend(c1, c2);
                    }
                }
            } else {
                xs = (lsx > x1) ? lsx - x1 : 0;
                for (int x = xs; x < width; x++) {
                    const Pixel c1 = *(src + x);
                    if (c1) *(dest + addr + x) = c1;
                }
            }
            addr += dpitch;
            src += m_Pitch;
        }
    }
}

void Sprite::DrawScaled(int a_X, int a_Y, int a_Width, int a_Height, Surface* a_Target)
{
    if ((a_Width == 0) || (a_Height == 0)) return;
    int v       = 0;
    int du      = (m_Pitch << 10) / a_Width;
    int dv      = (m_Height << 10) / a_Height;
    Pixel* dest = a_Target->GetBuffer() + a_X + a_Y * a_Target->GetPitch();
    Pixel* src  = GetBuffer() + m_CurrentFrame * m_Pitch;
    int x, y;
    for (y = 0; y < a_Height; y++) {
        int u  = 0;
        int cv = (v >> 10) * m_Pitch;
        for (x = 0; x < a_Width; x++) {
            *(dest + x) = *(src + (u >> 10) + cv);
            u += du;
        }
        v += dv;
        dest += a_Target->GetPitch();
    }
}

void Sprite::InitializeStartData()
{
    for (unsigned int f = 0; f < m_NumFrames; ++f) {
        m_Start[f] = new unsigned int[m_Height];
        for (int y = 0; y < m_Height; ++y) {
            m_Start[f][y] = m_Width;
            Pixel* addr   = GetBuffer() + f * m_Width + y * m_Pitch;
            for (int x = 0; x < m_Width; ++x) {
                if (addr[x]) {
                    m_Start[f][y] = x;
                    break;
                }
            }
        }
    }
}
