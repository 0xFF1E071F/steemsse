#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H


#if defined(SSE_VS2008)
#pragma warning(disable : 4710)
#endif

template <class Type> class DynamicArray
{
  Type *Data;
  int Size;
public:
  DynamicArray()      { Size=0;SizeInc=16;Data=NULL;NumItems=0; }
  DynamicArray(int n) { Size=n;SizeInc=16;Data=new Type[n];NumItems=0; }
  ~DynamicArray()     { if (Data) delete[] Data; }

//#if !defined(VC_BUILD) && !defined(UNIX)//SS
#if defined(BCC_BUILD)//SS
  Type& DynamicArray::operator[](int i) { return Data[i]; }
#endif
  operator Type*()        { return Data; }

  int NumItems;

  void Add(Type x)
  {
    if (NumItems>=Size) Resize(Size+SizeInc);
    Data[NumItems++]=x;
  }

  void Delete(int i)
  {
    if (i<NumItems && i>=0){
      if (i+1<NumItems) memmove(Data+i,Data+i+1,sizeof(Type)*(Size-i));
      NumItems--;
    }
  }

  void DeleteAll(bool resize=true)
  {
    NumItems=0;
    if (resize) Resize(0);
  }

  void Resize(int);

  int GetSize(){ return Size; }

  int SizeInc;
};

template <class Type> void DynamicArray<Type>::Resize(int NewSize)
{
  Type *NewData=NULL;
  if (NewSize) NewData=new Type[NewSize];
  if (Size>0 && NewSize>0) memcpy(NewData,Data,sizeof(Type)*min(Size,NewSize));
  if (Data) delete[] Data;
  Data=NewData;
  Size=NewSize;
  NumItems=min(Size,NumItems);
}

#endif

