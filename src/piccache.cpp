
// ��ȡidx/grp����ͼ�ļ���
// Ϊ����ٶȣ����û��淽ʽ��ȡ����idx/grp�����ڴ棬Ȼ�������ɸ��������
// �������ʵ�pic���ڻ��������

#include <stdlib.h>
#include "piccache.h"
#include "jymain.h"
#include "sdlfun.h"

PicFileCache pic_file[PIC_FILE_NUM];
//std::forward_list<CacheNode*> pic_cache;     //pic_cache����
Uint32 m_color32[256];               // 256��ɫ��
int g_EnableRLE = 0;
int CacheFailNum = 0;

void CacheNode::toTexture()
{
    if (s)
    {
        if (t == NULL)
        {
            t = SDL_CreateTextureFromSurface(g_Renderer, s);
            SDL_FreeSurface(s);
            s = NULL;
        }
    }
}

// ��ʼ��Cache���ݡ���Ϸ��ʼʱ����
int Init_Cache()
{
    int i;
    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        pic_file[i].idx = NULL;
        pic_file[i].grp = NULL;
        pic_file[i].fp = NULL;
        pic_file[i].pcache = NULL;
    }
    return 0;
}

// ��ʼ����ͼcache��Ϣ
// PalletteFilename Ϊ256��ɫ���ļ�����һ�ε���ʱ����
//                  Ϊ���ַ������ʾ���������ͼcache��Ϣ��������ͼ/����/ս���л�ʱ����

int JY_PicInit(char* PalletteFilename)
{
    struct list_head* pos, *p;
    int i;

    LoadPallette(PalletteFilename);   //�����ɫ��

    //���������Ϊ�գ�ɾ��ȫ������
    //for (auto& c : pic_cache)
    //{
    //    delete c;
    //}
    //pic_cache.clear();

    for (i = 0; i < PIC_FILE_NUM; i++)
    {
        pic_file[i].num = 0;
        SafeFree(pic_file[i].idx);
        SafeFree(pic_file[i].grp);
        SafeFree(pic_file[i].pcache);
        if (pic_file[i].fp)
        {
            fclose(pic_file[i].fp);
            pic_file[i].fp = NULL;
        }
    }
    CacheFailNum = 0;
    return 0;
}

// �����ļ���Ϣ
// filename �ļ���
// id  0 - PIC_FILE_NUM-1
int JY_PicLoadFile(const char* idxfilename, const char* grpfilename, int id, int width, int height)
{
    int i;
    struct CacheNode* tmpcache;
    FILE* fp;

    if (id < 0 || id >= PIC_FILE_NUM)    // id������Χ
    {
        return 1;
    }

    if (pic_file[id].pcache)          //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
    {
        int i;
        for (i = 0; i < pic_file[id].num; i++)     //ѭ��ȫ����ͼ��
        {
            tmpcache = pic_file[id].pcache[i];
            if (tmpcache)         // ����ͼ�л�����ɾ��
            {
                delete tmpcache;
            }
        }
        SafeFree(pic_file[id].pcache);
    }
    SafeFree(pic_file[id].idx);
    SafeFree(pic_file[id].grp);
    if (pic_file[id].fp)
    {
        fclose(pic_file[id].fp);
        pic_file[id].fp = NULL;
    }

    // ��ȡidx�ļ�

    pic_file[id].num = FileLength(idxfilename) / 4;    //idx ��ͼ����
    pic_file[id].idx = (int*)malloc((pic_file[id].num + 1) * 4);
    if (pic_file[id].idx == NULL)
    {
        JY_Error("JY_PicLoadFile: cannot malloc idx memory!\n");
        return 1;
    }
    //��ȡ��ͼidx�ļ�
    if ((fp = fopen(idxfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: idx file not open ---%s", idxfilename);
        return 1;
    }

    fread(&pic_file[id].idx[1], 4, pic_file[id].num, fp);
    fclose(fp);

    pic_file[id].idx[0] = 0;

    //��ȡgrp�ļ�
    pic_file[id].filelength = FileLength(grpfilename);

    //��ȡ��ͼgrp�ļ�
    if ((fp = fopen(grpfilename, "rb")) == NULL)
    {
        JY_Error("JY_PicLoadFile: grp file not open ---%s", grpfilename);
        return 1;
    }
    if (g_PreLoadPicGrp == 1)     //grp�ļ������ڴ�
    {
        pic_file[id].grp = (unsigned char*)malloc(pic_file[id].filelength);
        if (pic_file[id].grp == NULL)
        {
            JY_Error("JY_PicLoadFile: cannot malloc grp memory!\n");
            return 1;
        }
        fread(pic_file[id].grp, 1, pic_file[id].filelength, fp);
        fclose(fp);
    }
    else
    {
        pic_file[id].fp = fp;
    }


    pic_file[id].pcache = (struct CacheNode**)malloc(pic_file[id].num * sizeof(struct CacheNode*));
    if (pic_file[id].pcache == NULL)
    {
        JY_Error("JY_PicLoadFile: cannot malloc pcache memory!\n");
        return 1;
    }

    for (i = 0; i < pic_file[id].num; i++)
    { pic_file[id].pcache[i] = NULL; }

    if (height == 0)
    { height = width; }

    if (width > 0)
    {
        pic_file[id].width = width;
        pic_file[id].height = height;
    }

    return 0;
}

int JY_LoadPic(int fileid, int picid, int x, int y, int flag, int value)
{
    return JY_LoadPicColor(fileid, picid, x, y, flag, value, 0);
}

// ���ز���ʾ��ͼ
// fileid        ��ͼ�ļ�id
// picid     ��ͼ���
// x,y       ��ʾλ��
//  flag ��ͬbit������ͬ���壬ȱʡ��Ϊ0
//  B0    0 ����ƫ��xoff��yoff��=1 ������ƫ����
//  B1    0     , 1 �뱳��alpla �����ʾ, value Ϊalphaֵ(0-256), 0��ʾ͸��
//  B2            1 ȫ��
//  B3            1 ȫ��
//  value ����flag���壬Ϊalphaֵ��

int JY_LoadPicColor(int fileid, int picid, int x, int y, int flag, int value, int color)
{

    struct CacheNode* newcache, *tmpcache;
    int xnew, ynew;
    SDL_Surface* tmpsur;

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // ��������
    { return 1; }

    if (pic_file[fileid].pcache[picid] == NULL)     //��ǰ��ͼû�м���
    {
        //����cache����
        newcache = (struct CacheNode*)malloc(sizeof(struct CacheNode));
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPic: cannot malloc newcache memory!\n");
            return 1;
        }

        newcache->id = picid;
        newcache->fileid = fileid;
        LoadPic(fileid, picid, newcache);
        ////ָ�����Ⱥ͸߶�
        //if (newcache->s != NULL && pic_file[fileid].width > 0 && pic_file[fileid].height > 0
        //    && pic_file[fileid].width != newcache->s->w && pic_file[fileid].height != newcache->s->h)
        //{
        //    double zoomx = (double)pic_file[fileid].width / newcache->s->w;
        //    double zoomy = (double)pic_file[fileid].height / newcache->s->h;

        //    if (zoomx < zoomy)
        //    {
        //        zoomy = zoomx;
        //    }
        //    else
        //    {
        //        zoomx = zoomy;
        //    }

        //    tmpsur = newcache->s;

        //    newcache->s = zoomSurface(tmpsur, zoomx, zoomy, SMOOTHING_OFF);

        //    newcache->xoff = (int)(zoomx * newcache->xoff);
        //    newcache->yoff = (int)(zoomy * newcache->yoff);
        //    //SDL_SetColorKey(newcache->s, SDL_TRUE, ConvertColor(g_MaskColor32));  //͸��ɫ
        //    SDL_FreeSurface(tmpsur);
        //}
        pic_file[fileid].pcache[picid] = newcache;
    }
    else     //�Ѽ�����ͼ
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)     //��ͼΪ�գ�ֱ���˳�
    {
        return 1;
    }

    if (flag & 0x00000001)
    {
        xnew = x;
        ynew = y;
    }
    else
    {
        xnew = x - newcache->xoff;
        ynew = y - newcache->yoff;
    }
    RenderTexture(newcache->t, xnew, ynew, flag, value, color);
    return 0;
}

// ������ͼ������
static int LoadPic(int fileid, int picid, struct CacheNode* cache)
{

    SDL_RWops* fp_SDL;
    int id1, id2;
    int datalong;
    unsigned char* p, *data;

    SDL_Surface* tmpsurf = NULL, *tmpsur;

    if (pic_file[fileid].idx == NULL)
    {
        JY_Error("LoadPic: fileid %d can not load?\n", fileid);
        return 1;
    }
    id1 = pic_file[fileid].idx[picid];
    id2 = pic_file[fileid].idx[picid + 1];

    // ����һЩ��������������޸����еĴ���
    if (id1 < 0)
    { datalong = 0; }

    if (id2 > pic_file[fileid].filelength)
    { id2 = pic_file[fileid].filelength; }

    datalong = id2 - id1;

    if (datalong > 0)
    {
        //��ȡ��ͼgrp�ļ����õ�ԭʼ����
        if (g_PreLoadPicGrp == 1)           //��Ԥ�������ڴ��ж�����
        {
            data = pic_file[fileid].grp + id1;
            p = NULL;
        }
        else         //û��Ԥ�������ļ��ж�ȡ
        {
            fseek(pic_file[fileid].fp, id1, SEEK_SET);
            data = (unsigned char*)malloc(datalong);
            p = data;
            fread(data, 1, datalong, pic_file[fileid].fp);
        }

        fp_SDL = SDL_RWFromMem(data, datalong);
        if (IMG_isPNG(fp_SDL) == 0)
        {
            int w, h;
            w = *(short*)data;
            h = *(short*)(data + 2);
            cache->xoff = *(short*)(data + 4);
            cache->yoff = *(short*)(data + 6);
            cache->w = w;
            cache->h = h;
            cache->t = CreatePicSurface32(data + 8, w, h, datalong - 8);
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_FreeSurface(cache->s);
            //cache->s = NULL;
        }
        else        //��ȡpng��ʽ
        {
            tmpsurf = IMG_LoadPNG_RW(fp_SDL);
            if (tmpsurf == NULL)
            {
                JY_Error("LoadPic: cannot create SDL_Surface tmpsurf!\n");
            }
            cache->xoff = tmpsurf->w / 2;
            cache->yoff = tmpsurf->h;
            cache->w = tmpsurf->w;
            cache->h = tmpsurf->h;
            cache->s = tmpsurf;
            cache->toTexture();
            //cache->t = SDL_CreateTextureFromSurface(g_Renderer, cache->s);
            //SDL_FreeSurface(cache->s);
            //cache->s = NULL;
        }
        SDL_FreeRW(fp_SDL);
        SafeFree(p);
    }
    else
    {
        cache->s = NULL;
        cache->t = NULL;
        cache->xoff = 0;
        cache->yoff = 0;
    }

    return 0;
}


//�õ���ͼ��С
int JY_GetPicXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    struct CacheNode* newcache;
    int r = JY_LoadPic(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0);   //������ͼ����������λ��

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    { return 1; }

    newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)        // ���У���ֱ����ʾ
    {
        *w = newcache->w;
        *h = newcache->h;
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }

    return 0;
}

//����ԭ����Ϸ��RLE��ʽ��������
SDL_Texture* CreatePicSurface32(unsigned char* data, int w, int h, int datalong)
{
    int p = 0;
    int i, j;
    int yoffset;
    int row;
    int start;
    int x;
    int solidnum;
    SDL_Surface* ps1, *ps2;
    Uint32* data32 = NULL;

    data32 = (Uint32*)malloc(w * h * 4);
    if (data32 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot malloc data32 memory!\n");
        return NULL;
    }

    for (i = 0; i < w * h; i++)
    { data32[i] = 0; }

    for (i = 0; i < h; i++)
    {
        yoffset = i * w;
        row = data[p];            // i�����ݸ���
        start = p;
        p++;
        if (row > 0)
        {
            x = 0;                // i��Ŀǰ��
            for (;;)
            {
                x = x + data[p];    // i�пհ׵����������͸����
                if (x >= w)        // i�п��ȵ�ͷ������
                { break; }

                p++;
                solidnum = data[p];  // ��͸�������
                p++;
                for (j = 0; j < solidnum; j++)
                {
                    if (g_Rotate == 0)
                    {
                        data32[yoffset + x] = m_color32[data[p]] | AMASK;
                    }
                    else
                    {
                        data32[h - i - 1 + x * h] = m_color32[data[p]] | AMASK;
                    }
                    p++;
                    x++;
                }
                if (x >= w)
                { break; }     // i�п��ȵ�ͷ������
                if (p - start >= row)
                { break; }    // i��û�����ݣ�����
            }
            if (p + 1 >= datalong)
            { break; }
        }
    }
    ps1 = SDL_CreateRGBSurfaceFrom(data32, w, h, 32, w * 4, RMASK, GMASK, BMASK, AMASK);  //����32λ����
    if (ps1 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps1!\n");
    }
    ps2 = SDL_ConvertSurfaceFormat(ps1, g_Surface->format->format, 0);
    if (ps2 == NULL)
    {
        JY_Error("CreatePicSurface32: cannot create SDL_Surface ps2!\n");
    }
    auto tex = SDL_CreateTextureFromSurface(g_Renderer, ps2);
    SDL_FreeSurface(ps1);
    SDL_FreeSurface(ps2);
    SafeFree(data32);
    return tex;
}

// ��ȡ��ɫ��
// �ļ���Ϊ����ֱ�ӷ���
static int LoadPallette(char* filename)
{
    FILE* fp;
    char color[3];
    int i;
    if (strlen(filename) == 0)
    { return 1; }
    if ((fp = fopen(filename, "rb")) == NULL)
    {
        JY_Error("Pallette File not open ---%s", filename);
        return 1;
    }
    for (i = 0; i < 256; i++)
    {
        fread(color, 1, 3, fp);
        m_color32[i] = color[0] * 4 * 65536l + color[1] * 4 * 256 + color[2] * 4 + 0x000000;

    }
    fclose(fp);

    return 0;
}


int JY_LoadPNGPath(const char* path, int fileid, int num, int percent, const char* suffix)
{
    int i;
    struct CacheNode* tmpcache;
    if (fileid < 0 || fileid >= PIC_FILE_NUM)    // id������Χ
    {
        return 1;
    }

    if (pic_file[fileid].pcache)          //�ͷŵ�ǰ�ļ�ռ�õĿռ䣬������cache
    {
        int i;
        for (i = 0; i < pic_file[fileid].num; i++)     //ѭ��ȫ����ͼ��
        {
            tmpcache = pic_file[fileid].pcache[i];
            if (tmpcache)         // ����ͼ�л�����ɾ��
            {
                delete tmpcache;
            }
        }
        SafeFree(pic_file[fileid].pcache);
    }

    pic_file[fileid].num = num;
    sprintf(pic_file[fileid].path, "%s", path);

    pic_file[fileid].pcache = (struct CacheNode**)malloc(pic_file[fileid].num * sizeof(struct CacheNode*));
    if (pic_file[fileid].pcache == NULL)
    {
        JY_Error("JY_LoadPNGPath: cannot malloc pcache memory!\n");
        return 1;
    }
    for (i = 0; i < pic_file[fileid].num; i++)
    { pic_file[fileid].pcache[i] = NULL; }

    pic_file[fileid].percent = percent;
    sprintf(pic_file[fileid].suffix, "%s", suffix);

    return 0;
}

int JY_LoadPNG(int fileid, int picid, int x, int y, int flag, int value)
{

    struct CacheNode* newcache, *tmpcache;
    SDL_Surface* tmpsur;
    SDL_Rect r;

    picid = picid / 2;

    if (fileid < 0 || fileid >= PIC_FILE_NUM || picid < 0 || picid >= pic_file[fileid].num)    // ��������
    { return 1; }

    if (pic_file[fileid].pcache[picid] == NULL)     //��ǰ��ͼû�м���
    {
        char str[512];
        SDL_RWops* fp_SDL;
        double zoom = (double)pic_file[fileid].percent / 100.0;

        sprintf(str, "%s/%d.png", pic_file[fileid].path, picid);

        //����cache����
        newcache = (struct CacheNode*)malloc(sizeof(struct CacheNode));
        if (newcache == NULL)
        {
            JY_Error("JY_LoadPNG: cannot malloc newcache memory!\n");
            return 1;
        }

        newcache->id = picid;
        newcache->fileid = fileid;

        fp_SDL = SDL_RWFromFile(str, "rb");
        if (IMG_isPNG(fp_SDL))
        {
            tmpsur = IMG_LoadPNG_RW(fp_SDL);
            if (tmpsur == NULL)
            {
                JY_Error("JY_LoadPNG: cannot create SDL_Surface tmpsurf!\n");
                return 1;
            }

            newcache->xoff = tmpsur->w / 2;
            newcache->yoff = tmpsur->h / 2;
            newcache->w = tmpsur->w;
            newcache->h = tmpsur->h;
            newcache->s = tmpsur;
            newcache->toTexture();
            //newcache->t = SDL_CreateTextureFromSurface(g_Renderer, newcache->s);
            //SDL_FreeSurface(newcache->s);
            //newcache->s = NULL;
        }
        else
        {
            newcache->s = NULL;
            newcache->t = NULL;
            newcache->xoff = 0;
            newcache->yoff = 0;
        }

        SDL_FreeRW(fp_SDL);

        //ָ������
        if (pic_file[fileid].percent > 0 && pic_file[fileid].percent != 100 && zoom != 0 && zoom != 1)
        {
            newcache->w = (int)(zoom * newcache->w);
            newcache->h = (int)(zoom * newcache->h);
            //tmpsur = newcache->t;
            //newcache->s = zoomSurface(tmpsur, zoom, zoom, SMOOTHING_ON);
            newcache->xoff = (int)(zoom * newcache->xoff);
            newcache->yoff = (int)(zoom * newcache->yoff);
            //SDL_SetColorKey(newcache->s,SDL_SRCCOLORKEY|SDL_RLEACCEL ,ConvertColor(g_MaskColor32));  //͸��ɫ
            //SDL_FreeSurface(tmpsur);
        }
        pic_file[fileid].pcache[picid] = newcache;
    }
    else     //�Ѽ�����ͼ
    {
        newcache = pic_file[fileid].pcache[picid];
    }

    if (newcache->t == NULL)     //��ͼΪ�գ�ֱ���˳�
    {
        return 1;
    }

    if (flag & 0x00000001)
    {
        r.x = x;
        r.y = y;
    }
    else
    {
        r.x = x - newcache->xoff;
        r.y = y - newcache->yoff;
    }

    //SDL_BlitSurface(newcache->s, NULL, g_Surface, &r);
    r.w = newcache->w;
    r.h = newcache->h;
    SDL_SetRenderTarget(g_Renderer, g_Texture);
    SDL_RenderCopy(g_Renderer, newcache->t, NULL, &r);

    return 0;
}


int JY_GetPNGXY(int fileid, int picid, int* w, int* h, int* xoff, int* yoff)
{
    struct CacheNode* newcache;
    int r = JY_LoadPNG(fileid, picid, g_ScreenW + 1, g_ScreenH + 1, 1, 0);   //������ͼ����������λ��

    *w = 0;
    *h = 0;
    *xoff = 0;
    *yoff = 0;

    if (r != 0)
    { return 1; }

    newcache = pic_file[fileid].pcache[picid / 2];

    if (newcache->t)        // ���У���ֱ����ʾ
    {
        *w = newcache->w;
        *h = newcache->h;
        *xoff = newcache->xoff;
        *yoff = newcache->yoff;
    }

    return 0;
}