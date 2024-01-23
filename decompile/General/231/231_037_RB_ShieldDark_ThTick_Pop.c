#include <common.h>

void DECOMP_RB_ShieldDark_ThTick_Pop(struct Thread* t)
{
  struct Shield* sh;
  struct Instance* instDark;
  struct Instance* instColor;
  struct Driver* driverOwner;
  short rot[3];
  
  sh = t->object;
  instDark = t->inst;
  instColor = sh->instColor;
  driverOwner = t->parentThread->object;
  
  rot[0] = 0;
  rot[1] = 0;
  rot[2] = 0;
  LHMatrix_Parent(instDark, driverOwner->instSelf,&local_18);
  LHMatrix_Parent(instColor,driverOwner->instSelf,&local_18);
  
  // set rotation
  *(int*)&instDark->matrix.m[0][0] = 0x1000;
  *(int*)&instDark->matrix.m[0][2] = 0;
  *(int*)&instDark->matrix.m[1][1] = 0x1000;
  *(int*)&instDark->matrix.m[2][0] = 0;
  instDark->matrix.m[2][2] = 0x1000;
  
  // set rotation
  *(int*)&instColor->matrix.m[0][0] = 0x1000;
  *(int*)&instColor->matrix.m[0][2] = 0;
  *(int*)&instColor->matrix.m[1][1] = 0x1000;
  *(int*)&instColor->matrix.m[2][0] = 0;
  instColor->matrix.m[2][2] = 0x1000;
  
  int animFrame = sh->animFrame;
  
  if (animFrame < 0xb) 
  {
	// set scale
    instDark->scale[0] = *(short *)0x800b2d14[animFrame*2+0];
    instDark->scale[1] = *(short *)0x800b2d14[animFrame*2+1];
    instDark->scale[2] = *(short *)0x800b2d14[animFrame*2+0];
	
	// set scale
    instColor->scale[0] = *(short *)0x800b2d14[animFrame*2+0];
    instColor->scale[1] = *(short *)0x800b2d14[animFrame*2+1];
    instColor->scale[2] = *(short *)0x800b2d14[animFrame*2+0];
	
    // next frame
    sh->animFrame += 1;
	
	return;
  }
  
  // === Animation Done ===
  
  // play 3D sound for "shield pop"
  PlaySound3D(0x58, instDark);
	
  INSTANCE_Death(instColor);
  INSTANCE_Death(sh->instHighlight);
	
  // this thread is now dead
  t->flags |= 0x800;
  
  return;
}
 