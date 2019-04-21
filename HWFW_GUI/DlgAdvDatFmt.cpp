﻿#include "stdafx.h"
#include "HWFW_GUI.hpp"

#define SUBITEM_USE_OFFSET      0

/************************************************************************/
/*  华为WHWH子项目                                                      */
/************************************************************************/
typedef struct _HuaWeiSubItemObject {
  //子项目是否初始化
  BOOL          bIsInit = FALSE;
  //子项目是否UImage格式
  BOOL          bIsImage = FALSE;
#if SUBITEM_USE_OFFSET
  //子项目相对于项目数据的偏移
  uint32_t      u32Offset = 0;
#endif
  //项目总大小 (HW_HDR + UIMG_HDR + UIMG_Data)
  size_t        u32TotalSize = 0;
  //指向华为数据 (xxx || uimage hdr + uimage data)
  LPCVOID       lpcHuaWeiData = NULL;
  //指向uimage数据 (uimage data)
  LPCVOID       lpcImageData = NULL;
  //完整的数据
  LPVOID        lpRawData = NULL;
  //节点句柄
  HTREEITEM     hItem = NULL;
  //华为头
  HW_HDR        hdrHuaWei;
  //UImage头
  UIMG_HDR      hdrImage;
} HWSUBITEM_OBJ, *PHWSUBITEM_OBJ;


static LPCVOID lpItemData = NULL;
static uint32_t u32DataSize = 0;
static uint32_t u32ItemIdx;
static HWND hDlgFmt = NULL;
static PHWSUBITEM_OBJ lpSubItem = NULL;
static PHWSUBITEM_OBJ lpCurrentItem = NULL;
static uint32_t nSubItem = 0;

static void Release()
{
  lpItemData = NULL;
  u32DataSize = 0;

  if (lpSubItem && nSubItem) {
    for (uint32_t i = 0; i < nSubItem; i++) {
      if (lpSubItem[i].lpRawData) {
        free(lpSubItem[i].lpRawData);
        lpSubItem[i].lpRawData = NULL;
      }
    }
  }

  free(lpSubItem);
  lpSubItem = NULL;
  lpCurrentItem = NULL;
  nSubItem = 0;
}

static void DetectAlignMode() {
  // TODO
}

static void EnableUBootGroup(HWND hDlg, BOOL blEnable)
{
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBHDRCRC), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBDATCRC), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBSIZE), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBTIME), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBTIME_RO), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBLOAD), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBEP), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_EDIT_UBNAME), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_CB_UBOS), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_CB_UBARCH), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_CB_UBIMG), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_CB_UBCOMP), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_BTN_UBEXP), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_BTN_UBIMP), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_BTN_UBCH), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_BTN_UBCD), blEnable);
  EnableWindow(GetDlgItem(hDlg, IDC_BTN_UBSAVE), blEnable);

  EnableWindow(GetDlgItem(hDlg, IDC_GB_UBOOT), blEnable);
}

static void UpdateDataView_WHWH(HWND hDlg)
{
  size_t stOut;
  tm _tm;
  WCHAR wsTemp[260];

  mbstowcs_s(&stOut, wsTemp, lpCurrentItem->hdrHuaWei.chItemVersion, sizeof(HW_HDR::chItemVersion));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_WHVER), wsTemp);

  swprintf_s(wsTemp, SF_HEX, lpCurrentItem->hdrHuaWei.u32Time);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_WHTIME), wsTemp);

  _localtime32_s(&_tm, &lpCurrentItem->hdrHuaWei.u32Time);
  swprintf_s(wsTemp, SF_DATE, _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_WHTIME_RO), wsTemp);

  if ((lpCurrentItem->hdrHuaWei.u32Type >= HW_SubItemType::hwType_Invalid) && (lpCurrentItem->hdrHuaWei.u32Type < HW_SubItemType::hwType_Limit))
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_WHTYPE), lpCurrentItem->hdrHuaWei.u32Type);
  else
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_WHTYPE), 0);

  swprintf_s(wsTemp, SF_HEX, lpCurrentItem->hdrHuaWei.u32RearSize);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_WHSIZE), wsTemp);

  swprintf_s(wsTemp, SF_HEX, lpCurrentItem->hdrHuaWei.u32RearCRC);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_WHCRC), wsTemp);
}

static void UpdateDataView_UIMG(HWND hDlg)
{
  size_t stOut;
  __time32_t _time32;
  tm _tm;
  WCHAR wsTemp[260];

  swprintf_s(wsTemp, SF_HEX, EndianSwap32(lpCurrentItem->hdrImage.ih_hcrc));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBHDRCRC), wsTemp);

  swprintf_s(wsTemp, SF_HEX, EndianSwap32(lpCurrentItem->hdrImage.ih_dcrc));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBDATCRC), wsTemp);

  swprintf_s(wsTemp, SF_HEX, EndianSwap32(lpCurrentItem->hdrImage.ih_size));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBSIZE), wsTemp);

  _time32 = EndianSwap32(lpCurrentItem->hdrImage.ih_time);
  swprintf_s(wsTemp, SF_HEX, _time32);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBTIME), wsTemp);

  _localtime32_s(&_tm, &_time32);
  swprintf_s(wsTemp, SF_DATE, _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday, _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBTIME_RO), wsTemp);

  mbstowcs_s(&stOut, wsTemp, lpCurrentItem->hdrImage.ih_name, sizeof(UIMG_HDR::ih_name));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBNAME), wsTemp);

  swprintf_s(wsTemp, SF_HEX, EndianSwap32(lpCurrentItem->hdrImage.ih_load));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBLOAD), wsTemp);

  swprintf_s(wsTemp, SF_HEX, EndianSwap32(lpCurrentItem->hdrImage.ih_ep));
  SetWindowTextW(GetDlgItem(hDlg, IDC_EDIT_UBEP), wsTemp);

  if (lpCurrentItem->hdrImage.ih_os < IH_OS::IH_OS_COUNT)
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBOS), lpCurrentItem->hdrImage.ih_os);
  else
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBOS), 0);

  if (lpCurrentItem->hdrImage.ih_arch < IH_ARCH::IH_ARCH_COUNT)
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBARCH), lpCurrentItem->hdrImage.ih_arch);
  else
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBARCH), 0);

  if (lpCurrentItem->hdrImage.ih_type < IH_TYPE::IH_TYPE_COUNT)
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBIMG), lpCurrentItem->hdrImage.ih_type);
  else
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBIMG), 0);

  if (lpCurrentItem->hdrImage.ih_comp < IH_COMP::IH_COMP_COUNT)
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBCOMP), lpCurrentItem->hdrImage.ih_comp);
  else
    ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_CB_UBCOMP), 0);
}

static void ClearView() {
  //HW Group
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_WHVER), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_WHTIME), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_WHTIME_RO), L"");
  ComboBox_SetCurSel(GetDlgItem(hDlgFmt, IDC_CB_WHTYPE), 0);
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_WHSIZE), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_WHCRC), L"");

  //UBoot group
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBHDRCRC), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBDATCRC), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBSIZE), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBTIME), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBTIME_RO), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBNAME), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBLOAD), L"");
  SetWindowTextW(GetDlgItem(hDlgFmt, IDC_EDIT_UBEP), L"");
  ComboBox_SetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBOS), 0);
  ComboBox_SetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBARCH), 0);
  ComboBox_SetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBIMG), 0);
  ComboBox_SetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBCOMP), 0);
}

static BOOL UpdateDataView()
{
  ClearView();

  if (lpCurrentItem == NULL) return FALSE;

  EnableUBootGroup(hDlgFmt, lpCurrentItem->bIsImage);

  UpdateDataView_WHWH(hDlgFmt);
  if (lpCurrentItem->bIsImage) UpdateDataView_UIMG(hDlgFmt);

  return TRUE;
}

static void UpdateCurrent() {
  if (lpCurrentItem == NULL) return;

  if (lpCurrentItem->bIsInit && lpCurrentItem->lpRawData) {

  }
}

static int ImportData_WHWH() {
  LPVOID lpData = NULL;
  DWORD dwDataSize = 0;

  if (lpCurrentItem == NULL) return -1001;
  if (lpCurrentItem->bIsInit == FALSE) return -1002;

  __try {
    BOOL bIsImg;
    size_t size, sizeData;
    uint32_t index;
    LPBYTE lpMem;
    LPCVOID lpHuaWei = NULL, lpImage = NULL;
    PUIMG_HDR lpImgHdr = NULL;
    WCHAR wsTmp[MAX_PATH] = { 0 };

    if (GetOpenFilePath(hDlgFmt, wsTmp, MAX_PATH))
    {
      if (ImportFromFile(wsTmp, &lpData, &dwDataSize) == FALSE)
      {
        SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"打开文件失败,错误码:[%d]!", GetLastError());
        return -1;
      }

      bIsImg = FALSE;

      if (dwDataSize > sizeof(UIMG_HDR)) {
        lpImgHdr = (PUIMG_HDR)lpData;

        // 判断是否uImage
        if (lpImgHdr->ih_magic == IH_MAGIC_LE) {
          sizeData = EndianSwap32(lpImgHdr->ih_size);

          if (dwDataSize < sizeof(UIMG_HDR) + sizeData) {
            SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"uImage格式数据长度不合法!");
            return -2;
          }

          bIsImg = TRUE;
          size = sizeof(HW_HDR) + sizeof(UIMG_HDR) + sizeData;
        }
      }

      if (!bIsImg) {
        size = sizeof(HW_HDR) + dwDataSize;
      }

      lpMem = (LPBYTE)malloc(size);

      if (lpMem == NULL) {
        SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"分配内存失败!");
        return -3;
      }

      index = 0;

      memcpy_s(&lpMem[index], size - index, &lpCurrentItem->hdrHuaWei, sizeof(HW_HDR));
      index += sizeof(HW_HDR);

      lpHuaWei = &lpMem[index];

      if (bIsImg) {
        lpCurrentItem->hdrImage = *lpImgHdr;

        memcpy_s(&lpMem[index], size - index, lpImgHdr, sizeof(UIMG_HDR));
        index += sizeof(UIMG_HDR);

        lpImage = &lpMem[index];
        memcpy_s(&lpMem[index], size - index, MakePointer32(lpImgHdr, sizeof(UIMG_HDR)), sizeData);
      }
      else {
        memcpy_s(&lpMem[index], size - index, lpData, dwDataSize);
      }

      if (lpCurrentItem->lpRawData) free(lpCurrentItem->lpRawData);

      lpCurrentItem->bIsImage = bIsImg;
      lpCurrentItem->lpcHuaWeiData = lpHuaWei;
      lpCurrentItem->lpcImageData = lpImage;
      lpCurrentItem->lpRawData = lpMem;
      lpCurrentItem->hdrHuaWei.u32RearSize = size - sizeof(HW_HDR);
      lpCurrentItem->hdrHuaWei.u32RearCRC = crc32_fast(lpCurrentItem->lpcHuaWeiData, lpCurrentItem->hdrHuaWei.u32RearSize);
      lpCurrentItem->u32TotalSize = size;

      SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"导入WHWH数据完成.");
      UpdateDataView();
      return 1;
    }

    return 0;
  }
  __finally {
    if (lpData) free(lpData);
  }
}

static int ImportData_UIMG() {
  LPVOID lpData = NULL;
  DWORD dwDataSize = 0;

  if (lpCurrentItem == NULL) return -1001;
  if (lpCurrentItem->bIsInit == FALSE) return -1002;
  if (lpCurrentItem->bIsImage == FALSE) return -1003;

  __try {
    size_t size;
    uint32_t index;
    LPBYTE lpMem;
    LPCVOID lpHuaWei = NULL, lpImage = NULL;
    UIMG_HDR hdrImage;
    WCHAR wsTmp[MAX_PATH] = { 0 };

    if (GetOpenFilePath(hDlgFmt, wsTmp, MAX_PATH))
    {
      if (ImportFromFile(wsTmp, &lpData, &dwDataSize) == FALSE)
      {
        SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"打开文件失败,错误码:[%d]!", GetLastError());
        return -1;
      }

      size = sizeof(HW_HDR) + sizeof(UIMG_HDR) + dwDataSize;
      lpMem = (LPBYTE)malloc(size);

      if (lpMem == NULL) {
        SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"分配内存失败!");
        return -3;
      }

      //lpCurrentItem->lpImageData = lpData;

      hdrImage = lpCurrentItem->hdrImage;
      hdrImage.ih_size = EndianSwap32(dwDataSize);
      hdrImage.ih_dcrc = EndianSwap32(crc32_fast(lpData, dwDataSize));

      HWNP_CalibrationImageHeaderCrc32(&hdrImage);

      index = 0;

      //复制HW_HDR
      memcpy_s(&lpMem[index], size - index, &lpCurrentItem->hdrHuaWei, sizeof(HW_HDR));
      index += sizeof(HW_HDR);
      lpHuaWei = &lpMem[index];

      //复制UIMG_HDR
      memcpy_s(&lpMem[index], size - index, &hdrImage, sizeof(UIMG_HDR));
      index += sizeof(UIMG_HDR);
      lpImage = &lpMem[index];

      //复制uImage Data
      memcpy_s(&lpMem[index], size - index, lpData, dwDataSize);


      if (lpCurrentItem->lpRawData) free(lpCurrentItem->lpRawData);

      lpCurrentItem->lpcHuaWeiData = lpHuaWei;
      lpCurrentItem->lpcImageData = lpImage;
      lpCurrentItem->lpRawData = lpMem;
      lpCurrentItem->hdrImage = hdrImage;
      lpCurrentItem->hdrHuaWei.u32RearSize = size - sizeof(HW_HDR);
      lpCurrentItem->hdrHuaWei.u32RearCRC = crc32_fast(lpCurrentItem->lpcHuaWeiData, lpCurrentItem->hdrHuaWei.u32RearSize);
      lpCurrentItem->u32TotalSize = size;

      SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"导入UIMG数据完成.");
      UpdateDataView();
      return 1;
    }

    return 0;
  }
  __finally {
    if (lpData) free(lpData);
  }
}

static int MakeSubItem(LPVOID lpData, uint32_t maxSize, PHWSUBITEM_OBJ lpSubItem, BOOL align) {
  if (lpData == NULL) return -1;
  if (maxSize == 0) return -2;
  if (lpSubItem == NULL) return -3;
  if (lpSubItem->bIsInit == FALSE) return -4;
  if (maxSize < lpSubItem->u32TotalSize) return -5;

  uint32_t temp, offset;
  LPVOID lpItem;

  lpItem = NULL;

  __try {
    lpItem = malloc(lpSubItem->u32TotalSize);
    assert(lpItem != NULL);

    temp = 0;
    offset = 0;

    memcpy_s(MakePointer32(lpItem, offset), lpSubItem->u32TotalSize - offset, &(lpSubItem->hdrHuaWei), sizeof(HW_HDR));
    offset += sizeof(HW_HDR);

    if (lpSubItem->bIsImage) {
      memcpy_s(MakePointer32(lpItem, offset), lpSubItem->u32TotalSize - offset, &(lpSubItem->hdrImage), sizeof(UIMG_HDR));
      offset += sizeof(UIMG_HDR);

      temp = EndianSwap32(lpSubItem->hdrImage.ih_size);
      memcpy_s(MakePointer32(lpItem, offset), lpSubItem->u32TotalSize - offset, lpSubItem->lpcImageData, temp);
      offset += temp;
    }
    else {
      memcpy_s(MakePointer32(lpItem, offset), lpSubItem->u32TotalSize - offset, lpSubItem->lpcHuaWeiData, lpSubItem->hdrHuaWei.u32RearSize);
      offset += lpSubItem->hdrHuaWei.u32RearSize;
    }

    if (align) {
      temp = MemAlignCpy(lpData, maxSize, lpItem, offset, 0xFF);
    }
    else {
      temp = lpSubItem->u32TotalSize;
      memcpy_s(lpData, maxSize, lpItem, temp);
    }
  }
  __finally {
    free(lpItem);
  }

  return (int)temp;
}

/************************************************************************/
/* 保存子项目                                                           */
/* alignMode: 1,Margin; 2,Padding                                       */
/************************************************************************/
static int SaveSubItem(int alignMode) {
  if (lpSubItem == NULL) return -1;
  if (nSubItem == 0) return -2;

  int result, ret;
  uint32_t totalSize = 0, offset, index;
  LPVOID lpMem;

  for (uint32_t i = 0; i < nSubItem; i++) {
#define CURRENT     (lpSubItem[i])
    if (CURRENT.bIsInit == FALSE) break;

    totalSize += alignPage(CURRENT.u32TotalSize);
#undef CURRENT
  }

  if (totalSize == 0) return -3;

  lpMem = malloc(totalSize);

  index = 0;
  offset = 0;

  while (index < nSubItem && offset < totalSize) {
    __try {
#define CURRENT     (lpSubItem[index])
      if (CURRENT.bIsInit == FALSE) continue;

      ret = MakeSubItem(MakePointer32(lpMem, offset), totalSize - offset, &CURRENT, TRUE);

      if (ret < 0) {
        size_t stOut;
        WCHAR szText[MAX_PATH];
        WCHAR szTemp[MAX_PATH];

        mbstowcs_s(&stOut, szTemp, CURRENT.hdrHuaWei.chItemVersion, sizeof(HW_HDR::chItemVersion));
        wsprintf(szText, L"构造子项目出错: [%s]:[%d] !", szTemp, ret);
        MessageBoxW(hDlgFmt, szText, L"警告", MB_OK | MB_ICONWARNING);
      }
      else {
        offset += ret;
      }
#undef CURRENT
    }
    __finally {
      index++;
    }
  }

  result = HWNP_SetItemData(u32ItemIdx, lpMem, totalSize);

  free(lpMem);

  if (result != 0)
    SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"保存WHWH数据失败,错误码:[%d]!", result);
  else
    SetTooltip(GetDlgItem(hDlgFmt, IDC_LBL_ADF_STATUS), L"保存WHWH数据完成.");

  return result;
}

static void SaveHeader_WHWH()
{
  if (lpCurrentItem == NULL) return;

  CHAR chTmp[MAX_PATH] = { 0 };

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_WHVER), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) strcpy_s(lpCurrentItem->hdrHuaWei.chItemVersion, chTmp);

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_WHTIME), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) lpCurrentItem->hdrHuaWei.u32Time = ScanfHex(chTmp);

  lpCurrentItem->hdrHuaWei.u32Type = (HW_SubItemType)ComboBox_GetCurSel(GetDlgItem(hDlgFmt, IDC_CB_WHTYPE));
}

static void SaveHeader_UIMG()
{
  if (lpCurrentItem == NULL) return;
  if (lpCurrentItem->bIsImage == FALSE) return;

  uint32_t crc32;
  CHAR chTmp[MAX_PATH] = { 0 };

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_UBTIME), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) lpCurrentItem->hdrImage.ih_time = EndianSwap32(ScanfHex(chTmp));

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_UBLOAD), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) lpCurrentItem->hdrImage.ih_load = EndianSwap32(ScanfHex(chTmp));

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_UBEP), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) lpCurrentItem->hdrImage.ih_ep = EndianSwap32(ScanfHex(chTmp));

  GetWindowTextA(GetDlgItem(hDlgFmt, IDC_EDIT_UBNAME), chTmp, MAX_PATH);
  if (strlen(chTmp) != 0) strcpy_s(lpCurrentItem->hdrImage.ih_name, chTmp);

  lpCurrentItem->hdrImage.ih_os = (IH_OS)ComboBox_GetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBOS));
  lpCurrentItem->hdrImage.ih_arch = (IH_ARCH)ComboBox_GetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBARCH));
  lpCurrentItem->hdrImage.ih_type = (IH_TYPE)ComboBox_GetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBIMG));
  lpCurrentItem->hdrImage.ih_comp = (IH_COMP)ComboBox_GetCurSel(GetDlgItem(hDlgFmt, IDC_CB_UBCOMP));

  HWNP_CalibrationImageHeaderCrc32(&lpCurrentItem->hdrImage);

  // 修正HW_HDR CRC32
  crc32 = 0;
  crc32 = crc32_fast(&lpCurrentItem->hdrImage, sizeof(UIMG_HDR), crc32);
  crc32 = crc32_fast(lpCurrentItem->lpcImageData, lpCurrentItem->hdrHuaWei.u32RearSize - sizeof(UIMG_HDR), crc32);
  lpCurrentItem->hdrHuaWei.u32RearCRC = crc32;

  /*
  crc32 = 0;
  crc32 = crc32_fast(&lpCurrentItem->hdrImage, sizeof(UIMG_HDR), crc32);
  crc32 = crc32_fast(lpCurrentItem->lpcImageData, EndianSwap32(lpCurrentItem->hdrImage.ih_size), crc32);
  lpCurrentItem->hdrHuaWei.u32RearCRC = crc32;
  */
}


/************************************************************************/
/* 保存全部                                                             */
/************************************************************************/
static void SaveAll() {
  if (lpCurrentItem && lpCurrentItem->bIsImage)
  {
    SaveHeader_WHWH();
    SaveHeader_UIMG();
  }
  else
  {
    SaveHeader_WHWH();
  }

  SaveSubItem(1);
}

/************************************************************************/
/* 枚举WHWH子项目的数量                                                 */
/************************************************************************/
static uint32_t EnumSubItem() {
  const BYTE *lpData = (const BYTE*)lpItemData;
  const HW_HDR *lpHeader;
  uint32_t result = 0;
  uint32_t offset = 0;

  if (lpData == NULL) return result;

  //判断头长度是否满足
  while (offset + sizeof(HW_HDR) < u32DataSize) {
    lpHeader = (const PHW_HDR)(&lpData[offset]);

    if (lpHeader->u32Magic != HWNP_HWHW_MAGIC) break;

    result++;

    offset += alignPage(sizeof(HW_HDR) + lpHeader->u32RearSize);
  }

  return result;
}

static int InitSubItem(PHWSUBITEM_OBJ lpCurrent, LPCVOID lpData) {
  if (lpCurrent == NULL) return -1;
  if (lpData == NULL) return -2;

  const HW_HDR *lpHwHdr = (PHW_HDR)lpData;
  const UIMG_HDR *lpImgHdr;

  if (lpHwHdr->u32Magic != HWNP_HWHW_MAGIC) return -3;

  lpCurrent->hdrHuaWei = *lpHwHdr;
  lpCurrent->bIsImage = FALSE;
  lpCurrent->u32TotalSize = sizeof(HW_HDR) + lpHwHdr->u32RearSize;

  lpImgHdr = NULL;

  // 判断是否UImage
  if (lpHwHdr->u32RearSize >= sizeof(UIMG_HDR)) {
    lpImgHdr = (PUIMG_HDR)(MakePointer32(lpData, sizeof(HW_HDR)));

    if (lpImgHdr->ih_magic == IH_MAGIC_LE) {
      uint32_t size = EndianSwap32(lpImgHdr->ih_size);

      // 判断UImage Data长度 + UImage Header长度是否越界
      if (lpHwHdr->u32RearSize - sizeof(UIMG_HDR) >= size) {
        lpCurrent->bIsImage = TRUE;
      }
    }
  }

  lpCurrent->lpcHuaWeiData = MakePointer32(lpData, sizeof(HW_HDR));

  // 如果存在UImage
  if (lpCurrent->bIsImage && lpImgHdr) {
    lpCurrent->hdrImage = *lpImgHdr;

    lpCurrent->lpcImageData = MakePointer32(lpCurrent->lpcHuaWeiData, sizeof(UIMG_HDR));
  }

  // 标记该项目已初始化
  lpCurrent->bIsInit = TRUE;

  return alignPage((int)(sizeof(HW_HDR) + lpHwHdr->u32RearSize));
}

/************************************************************************/
/* 初始化每个子项目元素                                                 */
/************************************************************************/
static uint32_t InitSubItemList() {
  const BYTE *lpData = (const BYTE*)lpItemData;
  uint32_t offset = 0, index = 0;
  int ret;

  // 循环初始化每个子项目
  while ((offset + sizeof(HW_HDR) < u32DataSize) && (index < nSubItem)) {
#define CURRENT     (lpSubItem[index])
    ret = InitSubItem(&CURRENT, &lpData[offset]);
    if (ret < 0) break;

#if SUBITEM_USE_OFFSET
    CURRENT.u32Offset = offset;
#endif

    offset += alignPage(sizeof(HW_HDR) + CURRENT.hdrHuaWei.u32RearSize);

    index++;
#undef CURRENT
  }

  return index;
}

static uint32_t UpdateSubItemList() {
  uint32_t index = 0;
  TVINSERTSTRUCTA tvis;

  TreeView_DeleteAllItems(GetDlgItem(hDlgFmt, IDC_TV_SUBITEM));

  ZeroMemory(&tvis, sizeof(TVINSERTSTRUCTA));
  tvis.hParent = TVI_ROOT;
  tvis.hInsertAfter = TVI_LAST;
  tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

  for (index = 0; index < nSubItem; index++) {
#define CURRENT     (lpSubItem[index])
    if (CURRENT.bIsInit == FALSE) break;

    tvis.item.pszText = CURRENT.hdrHuaWei.chItemVersion;
    tvis.item.lParam = (LPARAM)index;
    CURRENT.hItem = TreeView_DlgInsertItemA(hDlgFmt, IDC_TV_SUBITEM, &tvis);
#undef CURRENT
  }

  return 0;
}

static int ParseSubItem() {
  uint32_t size;

  if (lpItemData == NULL) return -1;

  size = EnumSubItem();

  if (size == 0) return -2;

  nSubItem = size;
  lpSubItem = (PHWSUBITEM_OBJ)calloc(size, sizeof(HWSUBITEM_OBJ));

  InitSubItemList();
  UpdateSubItemList();
  lpCurrentItem = &lpSubItem[0];
  TreeView_DlgSelectItem(hDlgFmt, IDC_TV_SUBITEM, lpCurrentItem->hItem);

  return 0;
}

static void InitView() {
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_WHVER), EM_SETLIMITTEXT, sizeof(HW_HEADER::chItemVersion), 0);
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_WHTIME), EM_SETLIMITTEXT, 10, 0);
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_UBTIME), EM_SETLIMITTEXT, 10, 0);
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_UBLOAD), EM_SETLIMITTEXT, 10, 0);
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_UBEP), EM_SETLIMITTEXT, 10, 0);
  SNDMSG(GetDlgItem(hDlgFmt, IDC_EDIT_UBNAME), EM_SETLIMITTEXT, IH_NMLEN, 0);

  for (int i = 0; i < HW_SubItemType::hwType_Limit; i++) {
    ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_WHTYPE), i, HW_SubItemType_Text[i]);
  }

  ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_ALIGN), 0, "None");
  ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_ALIGN), 1, "Margin");
  ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_ALIGN), 2, "Padding");

  for (DWORD i = 0; i < IH_OS::IH_OS_COUNT; i++)
  {
    ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_UBOS), i, enum_IH_OS[i]);
  }

  for (DWORD i = 0; i < IH_ARCH::IH_ARCH_COUNT; i++)
  {
    ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_UBARCH), i, enum_IH_ARCH[i]);
  }

  for (DWORD i = 0; i < IH_TYPE::IH_TYPE_COUNT; i++)
  {
    ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_UBIMG), i, enum_IH_TYPE[i]);
  }

  for (DWORD i = 0; i < IH_COMP::IH_COMP_COUNT; i++)
  {
    ComboBox_InsertStringA(GetDlgItem(hDlgFmt, IDC_CB_UBCOMP), i, enum_IH_COMP[i]);
  }
}

static INT_PTR InitDlg(HWND hDlg, uint32_t nIndex) {
  DWORD dwType;
  int ret;
  HWNP_ITEMINFO hwItemInfo;

  Release();
  InitView();

  ret = HWNP_GetItemInfoByIndex(nIndex, &hwItemInfo);

  if (ret != 0) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"获取项目信息失败:[%d].", ret);
    return TRUE;
  }
  else
  {
    size_t stOut;
    WCHAR szText[MAX_PATH];
    WCHAR szPath[MAX_PATH];

    mbstowcs_s(&stOut, szPath, hwItemInfo.chItemPath, sizeof(HWNP_ITEMINFO::chItemPath));
    wsprintf(szText, L"高级项目数据格式编辑 => [%s]", szPath);

    SetWindowTextW(hDlg, szText);
  }

  ret = HWNP_GetItemDataSizeByIndex(nIndex, &u32DataSize);

  if (ret != 0) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"获取项目数据大小失败:[%d].", ret);
    return TRUE;
  }

  ret = HWNP_GetItemDataPointerByIndex(nIndex, &lpItemData);

  if (ret != 0) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"获取项目数据偏移失败:[%d].", ret);
    return TRUE;
  }


  ret = HWNP_GetItemDataTypeByIndex(nIndex, &dwType);

  if (ret != 0) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"获取项目数据类型失败:[%d].", ret);
    return TRUE;
  }


  if (!(CHK_FLAGS(dwType, IDT_WHWH))) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"项目数据类型不匹配!");
    return TRUE;
  }

  if (u32DataSize <= sizeof(HW_HDR)) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"项目数据长度太小!");
    return TRUE;
  }

  if (lpItemData == NULL) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"项目数据指针不合法!");
    return TRUE;
  }

  ret = ParseSubItem();

  if (ret != 0) {
    SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"解析子项目失败:[%d]!", ret);
    return TRUE;
  }

  u32ItemIdx = nIndex;


  if (UpdateDataView() == FALSE)
  {
    Release();
    MessageBoxW(hDlg, L"解析高级数据结构出现错误!", L"警告", MB_ICONWARNING | MB_OK);
    EndDialog(hDlg, 0);
    return (INT_PTR)FALSE;
  }

  /*
  if (u32DataSize < sizeof(HW_HDR) + hdrWHWH.u32RearSize)
  {
    //EnableWindow(hDlg, FALSE);
    MessageBoxW(hDlg, L"WHWH数据长度大于项目数据长度!", L"错误", MB_ICONERROR | MB_OK);
    return (INT_PTR)TRUE;
  }
  */

  return (INT_PTR)TRUE;
}

void SubItemChanged(LPNMTREEVIEW lpnmTV)
{
  uint32_t index = (uint32_t)lpnmTV->itemNew.lParam;

  if (index < nSubItem) {
    lpCurrentItem = &lpSubItem[index];
  }
  else {
    lpCurrentItem = NULL;
  }

  UpdateDataView();
}

/*
  Advanced data format dialog proc
*/
INT_PTR CALLBACK DlgProc_AdvDatFmt(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    hDlgFmt = hDlg;
    return InitDlg(hDlg, (uint32_t)lParam);

  case WM_SHOWWINDOW:
    if ((wParam) && (lParam == SW_PARENTOPENING)) UpdateDataView();
    break;

  case WM_COMMAND:
  {
    WORD wId = LOWORD(wParam), wNc = HIWORD(wParam);

    if (lParam != 0)
    {
      switch (wId)
      {
        //检查WHWH数据CRC32
      case IDC_BTN_WHCHK:
      {
        uint32_t u32CRC;

        if (lpCurrentItem) {
          u32CRC = crc32_fast(lpCurrentItem->lpcHuaWeiData, lpCurrentItem->hdrHuaWei.u32RearSize);

          if (u32CRC == lpCurrentItem->hdrHuaWei.u32RearCRC)
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"WHWH数据CRC32检查正确.");
          else
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"WHWH数据CRC32检查不正确, 计算结果:0x%08X!", u32CRC);
        }
        else {
          SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"无效的子项目!");
        }

      }
      break;

      //检查UIMG头部CRC32
      case IDC_BTN_UBCH:
        if (lpCurrentItem && lpCurrentItem->bIsImage)
        {
          uint32_t u32CRC;
          UIMG_HDR hdrTmp;

          hdrTmp = lpCurrentItem->hdrImage;
          hdrTmp.ih_hcrc = 0;

          u32CRC = crc32_fast(&hdrTmp, sizeof(UIMG_HDR));

          if (u32CRC == EndianSwap32(lpCurrentItem->hdrImage.ih_hcrc))
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"UIMG头部CRC32检查正确.");
          else
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"UIMG头部CRC32检查不正确, 计算结果:%08X (LE)!", u32CRC);
        }
        break;

        //检查UIMG数据CRC32
      case IDC_BTN_UBCD:
        if (lpCurrentItem && lpCurrentItem->bIsImage)
        {
          uint32_t u32CRC;

          u32CRC = crc32_fast(lpCurrentItem->lpcImageData, EndianSwap32(lpCurrentItem->hdrImage.ih_size));

          if (u32CRC == EndianSwap32(lpCurrentItem->hdrImage.ih_dcrc))
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"UIMG数据CRC32检查正确.");
          else
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"UIMG数据CRC32检查不正确, 计算结果:%08X (LE)!", u32CRC);
        }
        break;

        //导出WHWH数据
      case IDC_BTN_WHEXP:

        if (lpCurrentItem) {
          WCHAR wsTmp[MAX_PATH] = { 0 };

          if (GetSaveFilePath(hDlg, wsTmp, MAX_PATH))
          {
            if (ExportToFile(wsTmp, lpCurrentItem->lpcHuaWeiData, lpCurrentItem->hdrHuaWei.u32RearSize))
              SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出WHWH数据完成.");
            else
              SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出WHWH数据失败,错误码:[%d]!", GetLastError());
          }
        }

        break;

        //导入WHWH数据
      case IDC_BTN_WHIMP:

        if (lpCurrentItem) {
          SaveHeader_WHWH();
          ImportData_WHWH();
        }

        break;

        //导出UIMG数据
      case IDC_BTN_UBEXP:
        if (lpCurrentItem && lpCurrentItem->bIsImage)
        {
          WCHAR wsTmp[MAX_PATH] = { 0 };

          if (GetSaveFilePath(hDlg, wsTmp, MAX_PATH))
          {
            if (ExportToFile(wsTmp, lpCurrentItem->lpcImageData, EndianSwap32(lpCurrentItem->hdrImage.ih_size)))
              SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出UIMG数据完成.");
            else
              SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出UIMG数据失败,错误码:[%d]!", GetLastError());
          }
        }
        break;

        //导入UIMG数据
      case IDC_BTN_UBIMP:
        if (lpCurrentItem && lpCurrentItem->bIsImage)
        {
          SaveHeader_WHWH();
          SaveHeader_UIMG();
          ImportData_UIMG();
        }
        break;

        //保存WHWH 头部
      case IDC_BTN_WHSAVE:
        SaveHeader_WHWH();
        break;

        //保存UIMG 头部
      case IDC_BTN_UBSAVE:
        SaveHeader_UIMG();
        break;

        //保存WHWH 头部 + UIMG 头部 + 数据
      case IDC_BTN_ADF_SAVE:
        SaveAll();
        Release();
        EndDialog(hDlg, IDOK);
        break;

        //返回上级窗口
      case IDC_BTN_ADF_BACK:
        Release();
        EndDialog(hDlg, IDCANCEL);
        break;

      //添加华为子项目
      case IDC_BTN_ADD_HW_ITEM:
      {
        LPVOID lpData = NULL;
        DWORD dwSize = 0;
        WCHAR szFile[MAX_PATH] = { 0 };

        if (GetOpenFilePath(hDlg, szFile, MAX_PATH))
        {
          PHW_HEADER lpHwHdr;
          PHWSUBITEM_OBJ lpNewSubItem;
          int ret;


          if (!ImportFromFile(szFile, &lpData, &dwSize)) {
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导入文件数据失败!");
            if (lpData != NULL) free(lpData);
            break;
          }

          if (dwSize <= sizeof(HW_HEADER)) {
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"文件大小不合法!");
            if (lpData != NULL) free(lpData);
            break;
          }

          lpHwHdr = (PHW_HEADER)lpData;

          if (lpHwHdr->u32Magic != HWNP_HWHW_MAGIC) {
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"文件头魔法字不正确!");
            if (lpData != NULL) free(lpData);
            break;
          }

          lpNewSubItem = (PHWSUBITEM_OBJ)_recalloc(lpSubItem, nSubItem + 1, sizeof(HWSUBITEM_OBJ));

          if (lpNewSubItem == NULL) {
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"重新分配内存失败!");
            if (lpData != NULL) free(lpData);
            break;
          }

          ret = InitSubItem(&lpNewSubItem[nSubItem], lpData);

          if (ret < 0) {
            SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"解析对象失败: [%d]!", ret);
            if (lpData != NULL) free(lpData);
            break;
          }

          lpNewSubItem[nSubItem].lpRawData = lpData;

          nSubItem++;
          lpSubItem = lpNewSubItem;
          UpdateSubItemList();
          lpCurrentItem = &lpSubItem[0];
          TreeView_DlgSelectItem(hDlgFmt, IDC_TV_SUBITEM, lpCurrentItem->hItem);
        }
      }
      break;

      //导出选中子项目
      case IDC_BTN_EXP_HW_ITEM:
      {
        //size_t stOut;
        int ret;
        WCHAR szFile[MAX_PATH] = { 0 };
        //WCHAR szTemp[MAX_PATH];

        if (lpCurrentItem == NULL || lpCurrentItem->bIsInit == FALSE) break;

        //mbstowcs_s(&stOut, szTemp, lpCurrentItem->hdrHuaWei.chItemVersion, sizeof(HW_HDR::chItemVersion));

        wsprintf(szFile, L"sub.bin");

        if (GetSaveFilePath(hDlg, szFile, MAX_PATH)) {
          LPVOID lpData = malloc(lpCurrentItem->u32TotalSize);

          __try {
            ret = MakeSubItem(lpData, lpCurrentItem->u32TotalSize, lpCurrentItem, FALSE);

            if (ret < 0) {
              SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"构造子对象失败: [%d]!", ret);
            }
            else {
              if (ExportToFile(szFile, lpData, ret)) {
                SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出子项目成功: [%s]", szFile);
              }
              else {
                SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"导出子项目失败 !!!");
              }
            }
          }
          __finally {
            if (lpData) free(lpData);
          }
        }
      }
      break;

      //删除选中子项目
      case IDC_BTN_DEL_HW_ITEM:
      {
        PHWSUBITEM_OBJ lpNewSubItem;

        if (lpCurrentItem == NULL) break;

        if (nSubItem <= 1) {
          SetTooltip(GetDlgItem(hDlg, IDC_LBL_ADF_STATUS), L"无法继续删除子项目, 一个项目中至少包含一个子项目!");
          break;
        }

        lpNewSubItem = (PHWSUBITEM_OBJ)calloc(nSubItem - 1, sizeof(HWSUBITEM_OBJ));

        for (uint32_t i = 0, index = 0; i < nSubItem; i++) {
          //跳过需要删除的子项目
          if (&lpSubItem[i] == lpCurrentItem) continue;

          memcpy_s(&lpNewSubItem[index], sizeof(HWSUBITEM_OBJ), &lpSubItem[i], sizeof(HWSUBITEM_OBJ));
          index++;
        }

        if (lpCurrentItem->lpRawData) free(lpCurrentItem->lpRawData);
        free(lpSubItem);

        nSubItem = nSubItem - 1;
        lpSubItem = lpNewSubItem;
        UpdateSubItemList();
        lpCurrentItem = &lpSubItem[0];
        TreeView_DlgSelectItem(hDlgFmt, IDC_TV_SUBITEM, lpCurrentItem->hItem);
      }
      break;
      }

    }
    break;
  }

  case WM_NOTIFY:
  {
    if (lParam == 0) break;

    LPNMHDR lpNm = (LPNMHDR)lParam;

    switch (lpNm->code)
    {
    case TVN_SELCHANGED:
      SubItemChanged((LPNMTREEVIEW)lParam);
      break;

    case NM_CUSTOMDRAW:
      if (wParam == IDC_TV_SUBITEM)
      {
        LPNMTVCUSTOMDRAW lpCustomDraw = (LPNMTVCUSTOMDRAW)lParam;

        switch (lpCustomDraw->nmcd.dwDrawStage)
        {
        case CDDS_PREPAINT:
          SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
          return TRUE;
        case CDDS_ITEMPREPAINT:
          if (CHK_FLAGS(lpCustomDraw->nmcd.uItemState, CDIS_SELECTED)) {
            lpCustomDraw->clrText = RGB(255, 40, 40);
            lpCustomDraw->clrTextBk = RGB(192, 192, 192);
          }
          else {
            lpCustomDraw->clrText = RGB(20, 20, 20);
            lpCustomDraw->clrTextBk = RGB(242, 242, 242);
          }

          //SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT);
          return TRUE;
        }
      }
      break;
    }

  }
  break;

  case WM_CLOSE:
    Release();
    hDlgFmt = NULL;
    EndDialog(hDlg, IDCLOSE);
    break;
  }

  return (INT_PTR)FALSE;
}
