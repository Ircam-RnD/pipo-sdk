/**
 *
 * @file RingBuffer.h
 * @author Norbert.Schnell, Diemo Schwarz
 * 
 * @brief ringbuffer class
 * 
 * Copyright (C) 2014 by ISMM, IRCAM – Centre Pompidou, Paris, France.
 * All rights reserved.
 * 
 */
#ifndef _RINGBUFFER_
#define _RINGBUFFER_

template <class T>
class RingBuffer
{
public:
  std::vector<T> vector;
  unsigned int width;
  unsigned int size;
  unsigned int index;
  bool filled;
    
public:
  RingBuffer (void) 
  : vector()
  {
    this->width = 1;
    this->size = 0;
    this->index = 0;
    this->filled = false;  
  };
    
  void resize (int width, int size)
  {
    this->vector.resize(width * size);
    this->width = width;
    this->size = size;
    this->index = 0;
    this->filled = false;
  };
    
  void reset (void)
  {
    this->index = 0;
    this->filled = false;
  };
    
  int input (T *values, unsigned int num, PiPoValue scale = 1.0)
  {  
    T *ringValues = &this->vector[this->index * this->width];
      
    if (num > this->width)
      num = this->width;
      
    /* copy frame */
    if (scale == 1.0)	/*TODO: some polymorphism and template magic to avoid this if clause */
      memcpy(ringValues, values, num * sizeof(T));
    else
    {
      for (unsigned int j = 0; j < num; j++)
	ringValues[j] = values[j] * scale;
    }
      
    /* zero pad this values */
    if (num < this->width)
      memset(ringValues + num, 0, (this->width - num) * sizeof(T));
      
    this->index++;
      
    if (this->index >= this->size)
    {
      filled = true;
      this->index = 0;
    }      
      
    if (this->filled)
      return this->size;
    else
      return this->index;    
  };  
};

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset:2
 * End:
 */

#endif /* _RINGBUFFER_ */