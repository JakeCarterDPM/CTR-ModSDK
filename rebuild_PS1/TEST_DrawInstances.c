#include <common.h>


void TEST_DrawInstances(struct GameTracker* gGT)
{
	short posScreen1[4];
	short posWorld1[4];
	short posScreen2[4];
	short posWorld2[4];
	short posScreen3[4];
	short posWorld3[4];

	struct OTMem* otMem = &gGT->backBuffer->otMem;
	struct PrimMem* primMem = &gGT->backBuffer->primMem;

	for (
			struct Instance* curr = gGT->JitPools.instance.taken.first;
			curr != 0;
			curr = curr->next
		)
	{
		for (int i = 0; i < gGT->numPlyrCurrGame; i++)
		{
			if ((curr->flags & 0x80) != 0) continue;

			struct InstDrawPerPlayer* idpp = INST_GETIDPP(curr);

			struct TileView* view = idpp[i].tileView;
			if (view == 0) continue;

			// reset, might be off by one frame and glitch in the top-left corner,
			// when leaving character selection back to main menu
			//idpp[i].tileView = 0;

			// not this, this is identity matrix
#if 0
			MATRIX* mat1 = &view->matrix_ViewProj;
			*(int*)&mat1->m[0][0] = 0x1000;
			*(int*)&mat1->m[1][1] = 0x900; // -- temporary --
			*(int*)&mat1->m[2][2] = 0x1000;
#endif

			// copy from running CTR instance in no$psx,
			// INSTANCE IDPP offset 0x78 is a 4x4 MVP
			// INSTANCE IDPP offset 0x98 is a 3x3, idk
			MATRIX* mat2 = &curr->matrix;
			*(int*)&mat2->m[0][0] = 0x332;
			*(int*)&mat2->m[0][2] = 0x205;
			*(int*)&mat2->m[1][1] = 0xFDAC;
			*(int*)&mat2->m[2][0] = 0x13B;
			*(int*)&mat2->m[2][2] = 0xFFFFFAC2;
			mat2->t[0] = 0;
			mat2->t[1] = 0x58;
			mat2->t[2] = 0x320;

			// how do I multiply mat1 and mat2 together?
			gte_SetRotMatrix(mat2);
			gte_SetTransMatrix(mat2);

			struct Model* m = curr->model;
			struct ModelHeader* mh = &m->headers[0];
			char* vertData = (char*)&mh->ptrFrameData[0] + mh->ptrFrameData->vertexOffset;

			// 3FF is background, 3FE is next depth slot
			void* ot = &view->ptrOT[0x3FE];

			// test
#if 0
			POLY_F3* p1 = primMem->curr;
			primMem->curr = p1 + 1;

			// RGB
			*(int*)&p1->r0 = 0xFFFFFF;

			setPolyF3(p1);

			// to be in viewport, coordinates must be
			// X: [0, 0x40]
			// Y: [0, 0xA0]
			setXY3(p1,
				0x10, 0x10,	// XY0
				0x10, 0x38,	// XY1
				0x98, 0x38);	// XY2

			AddPrim(ot, p1);
			continue;
#endif

			//helper type, kinda same as RGB
			//a 255 grid "compressed vertex" 0 = 0.0 and 255 = 1.0. 256 steps only.
			typedef struct CompVertex {
				u_char X;
				u_char Y;
				u_char Z;
			} CompVertex;

			//flag values and end of list
#define END_OF_LIST 0xFFFFFFFF
#define DRAW_CMD_FLAG_NEW_STRIP (1 << 7)
#define DRAW_CMD_FLAG_SWAP_VERTEX (1 << 6)
#define DRAW_CMD_FLAG_FLIP_NORMAL (1 << 5)
#define DRAW_CMD_FLAG_CULLING (1 << 4)
#define DRAW_CMD_FLAG_COLOR_SCRATCHPAD (1 << 3)
#define DRAW_CMD_FLAG_NEW_VERTEX (1 << 2)
//bits 0 and 1 assumed unused

//variables
//sequentially point to the next vertex, increases once NEW_VERTEX flag comes in
			int vertexIndex = 0;
			//current strip length
			int stripLength = 0;
			CompVertex* ptrVerts = vertData;
			u_int* pCmd = mh->ptrCommandList;

			//a "shifting window", here we update the vertices and read triangle once it's ready
			//you need same cache for both colors and texture layouts
			CompVertex tempCoords[4];
			int tempColor[4];

			//i believe this must be scratchpad, but it uses 4 bytes, this array is only 3 bytes (like array buffer for simplicity).
			//the idea is that it loads vertices to scratchpad and with proper sorting,
			//you can draw may trigles of the list with minimum additional loads
			//then once you don't need vertex data, you can overwrite same indices with new data
			CompVertex stack[256];

			// pCmd[0] is number of commands
			pCmd++;

			//loop commands until we hit the end marker 
			while (*pCmd != END_OF_LIST)
			{
				//extract individual values from the command
				//refactor to a set of inline macros?
				u_short flags = (*pCmd >> (8 * 3)) & 0xFF; //8 bits
				u_short stackIndex = (*pCmd >> 16) & 0xFF; //8 bits
				u_short colorIndex = (*pCmd >> 9) & 0x7F; // 7 bits
				u_short texIndex = *pCmd & 0x1FF; //9 bits

				// if got a new vertex, load it
				if ((flags & DRAW_CMD_FLAG_NEW_VERTEX) == 0)
				{
					//copy from vertex buffer to stack index
					stack[stackIndex] = ptrVerts[vertexIndex];

					//and point to next vertex
					vertexIndex++;
				}

				//push current list back and insert value from stack
				//this code already have correct value on the stack, be aware of the order
				tempCoords[0] = tempCoords[1];
				tempCoords[1] = tempCoords[2];
				tempCoords[2] = tempCoords[3];
				tempCoords[3] = stack[stackIndex];

				//push new color
				tempColor[0] = tempColor[1];
				tempColor[1] = tempColor[2];
				tempColor[2] = tempColor[3];
				tempColor[3] = mh->ptrColors[colorIndex];

				//this is probably some tristrip optimization, so we can reuse vertex from the last triangle
				//and only spend 1 command
				if ((flags & DRAW_CMD_FLAG_SWAP_VERTEX) != 0)
				{
					tempCoords[1] = tempCoords[0];
					tempColor[1] = tempColor[0];
				}

				//if got reset flag, reset tristrip vertex counter
				if ((flags & DRAW_CMD_FLAG_NEW_STRIP) != 0)
				{
					stripLength = 0;
				}

				//enough data to add prim
				if (stripLength >= 2)
				{

					POLY_G3* p = primMem->curr;
					primMem->curr = p + 1;

					// RGB
					*(int*)&p->r0 = tempColor[1];
					*(int*)&p->r1 = tempColor[2];
					*(int*)&p->r2 = tempColor[3];

					// set Poly_LineF3 len, code, and padding
					setPolyG3(p);

					short zz;
					posWorld1[0] = ((mh->ptrFrameData->pos[0] + tempCoords[1].X) * mh->scale[0]) >> 8;
					posWorld1[1] = -((mh->ptrFrameData->pos[2] + tempCoords[1].Y) * mh->scale[2]) >> 8;
					posWorld1[2] = ((mh->ptrFrameData->pos[1] + tempCoords[1].Z) * mh->scale[1]) >> 8;
					posWorld1[3] = 0;
					zz = posWorld1[2];
					posWorld1[2] = -posWorld1[1];
					posWorld1[1] = zz;
					gte_ldv0(&posWorld1[0]);
					gte_rtps();
					gte_stsxy(&posScreen1[0]);

					posWorld2[0] = ((mh->ptrFrameData->pos[0] + tempCoords[2].X) * mh->scale[0]) >> 8;
					posWorld2[1] = -((mh->ptrFrameData->pos[2] + tempCoords[2].Y) * mh->scale[2]) >> 8;
					posWorld2[2] = ((mh->ptrFrameData->pos[1] + tempCoords[2].Z) * mh->scale[1]) >> 8;
					posWorld2[3] = 0;
					zz = posWorld2[2];
					posWorld2[2] = -posWorld2[1];
					posWorld2[1] = zz;
					gte_ldv0(&posWorld2[0]);
					gte_rtps();
					gte_stsxy(&posScreen2[0]);

					posWorld3[0] = ((mh->ptrFrameData->pos[0] + tempCoords[3].X) * mh->scale[0]) >> 8;
					posWorld3[1] = -((mh->ptrFrameData->pos[2] + tempCoords[3].Y) * mh->scale[2]) >> 8;
					posWorld3[2] = ((mh->ptrFrameData->pos[1] + tempCoords[3].Z) * mh->scale[1]) >> 8;
					posWorld3[3] = 0;
					zz = posWorld3[2];
					posWorld3[2] = -posWorld3[1];
					posWorld3[1] = zz;
					gte_ldv0(&posWorld3[0]);
					gte_rtps();
					gte_stsxy(&posScreen3[0]);

					// to be in viewport, coordinates must be
					// X: [0, 0x40]
					// Y: [0, 0xA0]
					setXY3(p,
						(posScreen1[0] / 2), (posScreen1[1] / 2),	// XY0
						(posScreen2[0] / 2), (posScreen2[1] / 2),	// XY1
						(posScreen3[0] / 2), (posScreen3[1] / 2));	// XY2

					AddPrim(ot, p);

					int x = *(int*)&tempCoords[0];

					if ((flags & DRAW_CMD_FLAG_FLIP_NORMAL) != 0)
					{
						//swap 2 coords to flip the normal here or can have 2 separate sumbission branches
					}
				}

				//strip length increases
				stripLength++;

				//proceed to the next command
				pCmd++;
			}
		}
	}
}