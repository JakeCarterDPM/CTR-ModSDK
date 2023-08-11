#include <common.h>

void DECOMP_Garage_LerpFX(void)
{
  int cVar1;
  struct GarageFX* garageSounds;
  short sVar5;
  short sVar6;
  short volume;
  short LR;
  int i;

  for (i = 0; i < 8; i++) 
  {
    garageSounds = &sdata->garageSoundPool[i];

    cVar1 = garageSounds->gsp_curr;
	
	if (cVar1 == GSP_CENTER) 
	{
      sVar6 = 0xff;
      sVar5 = 0x80;
    }
    
	else if (cVar1 == GSP_LEFT) 
	{
      sVar6 = 100;
      sVar5 = 0x3c;
    }
    
	else if (cVar1 == GSP_RIGHT) 
	{
      sVar6 = 100;
      sVar5 = 0xc3;
    }
	
    else 
	{
	  sVar6 = 0;
      sVar5 = garageSounds->LR;
    }

	LR = garageSounds->LR;
	volume = garageSounds->volume;

	// if change is desired, lerp properties
    if ((sVar5 != LR) || (sVar6 != volume)) 
	{
	  // desired audio change
      if (sVar6 != volume) 
	  {
		// lerp up
        if (volume < sVar6) 
		{
		  volume += 8;
          if(volume > sVar6)
			  volume = sVar6;
        }
        
		// lerp down
		else 
		{
          volume -= 8;
          if(volume < sVar6)
			  volume = sVar6;
        }
		
		garageSounds->volume = volume;
      }
      
	  // desired LR change
	  if (sVar5 != LR) 
	  {
		// lerp up
        if (LR < sVar5) 
		{
		  LR += 2;
          if(LR > sVar5)
			  LR = sVar5;
        }
		
		// lerp down
        else 
		{
          LR -= 2;
          if(LR < sVar5)
			  LR = sVar5;
        }
		
		garageSounds->LR = LR;
      }
	  
      if (sdata->garageSoundIDs[i] != 0) 
	  {
        OtherFX_RecycleNew(
            garageSounds->audioPtr,
            sdata->garageSoundIDs[i],
            (int)volume << 0x10 | (int)LR | 0x8000U
            );
      }
	  
	  // if desired properties have been reached
      if (((sVar5 == LR) && (sVar6 == volume)) &&
        (
            garageSounds->gsp_prev = garageSounds->gsp_curr,
            garageSounds->gsp_curr == GSP_GONE
        ))
	  {
        OtherFX_RecycleMute(garageSounds->audioPtr);
      }
    }
  } 
  return;
}