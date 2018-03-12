#ifndef SIMPLE_OBJ_H
#define SIMPLE_OBJ_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#define OBJ_COMMENT "#"
#define OBJ_VERTEX "v"
#define OBJ_FACE "f"
#define OBJ_VERTEX_NORMAL "vn"
#define OBJ_VERTEX_TEX_CORD "vt"
#define OBJ_GROUP "g"

/* Simple Obj Loader

   v0.2.0
   Author: MacDue
   License: Unlicense

   Loading and drawing Wavefront .obj files.
   Supports a subset of the full .obj formal.

   - Vertices (v)
   - Vertex normals (vn)
   - Texture coordinates (vt)
   - Faces (f)
   - Groups (g)
   - Simple uncached drawing
*/

typedef struct ObjDataArray {
  void* data;
  int len;
  int nextIndex;
  int typeSize;
} ObjDataArray_t;

typedef struct ObjFaceComponent {
  int vertexIndex;
  int texCoordIndex;
  int normalIndex;
} ObjFaceComponent_t;

typedef struct ObjVertex {
  double x, y, z, w;
  double r,g,b;
} ObjVertex_t;

typedef struct ObjTexCoord {
  double u, v, w;
} ObjTexCoord_t;

typedef struct ObjVertexNormal {
  double x, y, z;
} ObjVertexNormal_t;

typedef struct ObjGroup {
  char* name;
  int startFace;
  int endFace;
  bool render;
} ObjGroup_t;

typedef struct SimpleObj {
  ObjDataArray_t/*<ObjVertex_t>*/ vertices;
  ObjDataArray_t/*<ObjTexCoord_t>*/ texCoords;
  ObjDataArray_t/*<ObjVertexNormal_t>*/ normals;
  ObjDataArray_t/*<ObjDataArray_t<ObjFaceComponent_t>*/ faces;
  ObjDataArray_t/*<ObjGroup_t>*/ groups;
  // ObjDataArray_t/*<int>*/ lines; Not implemented.
} SimpleObj_t;

SimpleObj_t* loadObj(char* file_name);
void disposeObj(SimpleObj_t* obj);
void drawObj(SimpleObj_t* obj);
void* objDataArrayAccess(ObjDataArray_t* objDataArray, int index);

// Will write to returnArray.
#define STANDARD_ARRAY_PARSER(arrayType_t, strtoFunc)             \
static int                                                        \
parseArray_ ## arrayType_t                                        \
(char* arrayStr, int minSize, arrayType_t* returnArray) {         \
  replaceDelims(arrayStr);                                        \
  ObjDataArray_t doubleData = {};                                 \
  initObjDataArray(                                               \
    &doubleData, sizeof (arrayType_t), minSize, returnArray);     \
  char* next = NULL;                                              \
  char* previous = arrayStr;                                      \
  for (;;) {                                                      \
    arrayType_t d = strtoFunc;                                    \
    if (next == previous) {                                       \
      break;                                                      \
    }                                                             \
    previous = next;                                              \
    objDataArrayAppend(&doubleData,&d);                           \
  }                                                               \
  return doubleData.nextIndex;                                    \
}

#endif

#ifdef SIMPLE_OBJ_IMP

static const char extra_delims[] = {'/', '\0'};

// Sadly needed to fix strtol
static void replaceDelims(char* str) {
  int i = 0;
  while (str[i]) {
    int delmin = 0;
    while(extra_delims[delmin]) {
      if (str[i] == extra_delims[delmin++]) {
        str[i] = ' ';
      }
    }
    i++;
  }
}

// Needed to check trailing whitespace on faces
static bool strIsSpace(char* str) {
  while (*str) {
    if (!isspace(*str)) return false;
    str++;
  }
  return true;
}

/**
  Access a element in a one of the objs arrays.
  You must cast the result to the type you were accssing e.g. ObjVertex_t*.

  @param ObjDataArray_t* objDataArray An array e.g. obj->vertices
  @param int             index        An index into that array

  @return void* The accessed element
*/
inline void* objDataArrayAccess(ObjDataArray_t* objDataArray, int index) {
  return (void*)((size_t)objDataArray->data+index*objDataArray->typeSize);
}

static ObjDataArray_t*
objDataArrayAppend (ObjDataArray_t* objDataArray, void* data) {
  // Type safey is for the weak.
  if (objDataArray->nextIndex >= objDataArray->len) {
    int newLen = objDataArray->len * 2;
    objDataArray->data
      = realloc(objDataArray->data, newLen * objDataArray->typeSize);
    objDataArray->len = newLen;
  }
  // Must directly copy memory to avoid any casting.
  memcpy (
    objDataArrayAccess(objDataArray, objDataArray->nextIndex),
    data, objDataArray->typeSize
  );
  objDataArray->nextIndex++;
  return objDataArray;
}

static inline void objDataArrayDispose(ObjDataArray_t* objDataArray) {
  free(objDataArray->data);
}

static ObjDataArray_t*
initObjDataArray (ObjDataArray_t* objDataArray,
                  int typeSize, int reserve, void* array) {
  objDataArray->data = array == NULL ? malloc(typeSize * reserve) : array;
  objDataArray->len = reserve;
  objDataArray->nextIndex = 0;
  objDataArray->typeSize = typeSize;
  return objDataArray;
}

// Create some generic parsers
STANDARD_ARRAY_PARSER(double, strtod(previous, &next))
STANDARD_ARRAY_PARSER(long, strtol(previous, &next, 10))

static SimpleObj_t* newSimpleObj(void) {
  SimpleObj_t* obj = malloc(sizeof (SimpleObj_t));
  initObjDataArray(&obj->vertices, sizeof (ObjVertex_t), 100, NULL);
  initObjDataArray(&obj->texCoords, sizeof (ObjTexCoord_t), 100, NULL);
  initObjDataArray(&obj->normals, sizeof (ObjVertexNormal_t), 100, NULL);
  initObjDataArray(&obj->faces, sizeof (ObjDataArray_t), 100, NULL);
  // initObjDataArray(&obj->lines, sizeof (int*), 100, NULL);
  initObjDataArray(&obj->groups, sizeof (ObjGroup_t), 100, NULL);
  return obj;
}

static inline int fpeek(FILE* file) {
  int c = fgetc(file);
  ungetc(c, file);
  return c;
}

static long longDataBuffer[6];
static double dataInputBuffer[6];

/*
  Load a Wavefront .obj file.

  @param char* fileName A path to a .obj

  @return SimpleObj_t* a pointer to a loaded .obj
                       or NULL if the file could not be opened.
*/
SimpleObj_t* loadObj(char* fileName) {

  FILE *obj_fp;
  obj_fp = fopen (fileName, "r");
  if (obj_fp == NULL)
  {
    fprintf(stderr, "Unable to open obj file.\n");
    return NULL;
  }

  SimpleObj_t* obj = newSimpleObj();
  // strdup for ease of freeing
  ObjGroup_t currentGroup = { .name = strdup("Default"),
                              .startFace = 0,
                              .endFace = 0,
                              .render = true };

  size_t lineLen;
  char* line = NULL;
  size_t len = 0;
  // Only set when needed. Does not always contain correct value.
  bool peekEof = false;

  while ((lineLen = getline(&line, &len, obj_fp)) != -1) {
    char* lineType = strtok(line, " ");
    char* remaining = strtok(NULL, "\n");
    if (lineType == NULL || remaining == NULL) {
      /* Junk */
      continue;
    }
    if (strcmp(lineType, OBJ_GROUP) == 0
                || (peekEof = (fpeek(obj_fp) == EOF))) {
      /* Groups */
      if (obj->faces.nextIndex - currentGroup.startFace > 0) {
        currentGroup.endFace = obj->faces.nextIndex;
        objDataArrayAppend(&obj->groups, &currentGroup);
      } else {
        free(currentGroup.name);
      }
      if (!peekEof) {
        currentGroup.name = strdup(remaining);
        currentGroup.startFace = obj->faces.nextIndex;
      } else goto nextCase; // A hack. Continue down the if/else.
    } else nextCase: if (strcmp(lineType, OBJ_VERTEX) == 0) {
      /* Vertices */
      int vertexLen = parseArray_double(remaining, 9, dataInputBuffer);
      assert(vertexLen == 3 || vertexLen == 4
              || ("If coloured vertex" && vertexLen == 6));
      ObjVertex_t vertex = { .x = dataInputBuffer[0],
                             .y = dataInputBuffer[1],
                             .z = dataInputBuffer[2],
                             .w = vertexLen == 4 ? dataInputBuffer[3] : 1,
                             .r = -1, .g = -1, .b = -1};
      //printf("%d\n", vertexLen);
      if (vertexLen == 6) {
        vertex.r = dataInputBuffer[3];
        vertex.g = dataInputBuffer[4];
        vertex.b = dataInputBuffer[5];
      }
      objDataArrayAppend(&obj->vertices, &vertex);
    } else if (strcmp(lineType, OBJ_VERTEX_NORMAL) == 0) {
      /* Normals */
      int normalLen = parseArray_double(remaining, 3, dataInputBuffer);
      assert(normalLen == 3);
      ObjVertexNormal_t normal = { .x = dataInputBuffer[0],
                                   .y = dataInputBuffer[1],
                                   .z = dataInputBuffer[2] };
      objDataArrayAppend(&obj->normals, &normal);
    } else if (strcmp(lineType, OBJ_VERTEX_TEX_CORD) == 0) {
      /* Texture coordinates */
      int textCoordLen = parseArray_double(remaining, 3, dataInputBuffer);
      assert(textCoordLen == 2 || textCoordLen == 3);
      ObjTexCoord_t texCoord = { .u = dataInputBuffer[0],
                                 .v = dataInputBuffer[1],
                                 .w = textCoordLen > 2
                                        ? dataInputBuffer[2] : 0};
      objDataArrayAppend(&obj->texCoords, &texCoord);
    } else if (strcmp(lineType, OBJ_FACE) == 0) {
      /* Faces */
      ObjDataArray_t face = {};
      // Most faces seem to have at most 4 vertices.
      initObjDataArray(&face, sizeof(ObjFaceComponent_t), 4, NULL);
      bool vertexAndNormalOnly = strstr(remaining, "//");
      int vnIndexBuff = vertexAndNormalOnly ? 1 : 2;
      char* faceComponentStr = strtok(remaining, " ");
      while (faceComponentStr != NULL && !strIsSpace(faceComponentStr)) {
        int faceCompLen = parseArray_long(faceComponentStr, 3, longDataBuffer);
        assert(faceCompLen >= 1 && faceCompLen <= 3);
        // Length 1 = vertex
        // Length 2 = vertex//normal or vertex/tex
        // Length 3 = vertex/tex/normal
        ObjFaceComponent_t faceComponent = { .vertexIndex = longDataBuffer[0],
                                             .texCoordIndex
                                                = faceCompLen > 1
                                                    && !vertexAndNormalOnly
                                                  ? longDataBuffer[1] : -1,
                                             .normalIndex
                                                = vnIndexBuff < faceCompLen
                                                  ? longDataBuffer[vnIndexBuff]
                                                  : -1
                                            };
        objDataArrayAppend(&face, &faceComponent);
        faceComponentStr = strtok(NULL, " ");
      }
      objDataArrayAppend(&obj->faces, &face);
      if (peekEof) {
        // If the last line of the file is a face then the last group will
        // miss that face unless corrected here.
        ObjGroup_t* lastGroup
          = objDataArrayAccess(&obj->groups, obj->groups.nextIndex-1);
        lastGroup->endFace++;
      }
    }
    #ifdef DEBUG
      else if (strcmp(lineType, OBJ_COMMENT) != 0) {
      fprintf(stderr, "Unknown line type '%s'\n", lineType);
    }
    #endif
  }

  #ifdef DEBUG
  printf(
    "obj: vertices %d, texture coords %d, normals %d, faces %d, groups %d\n",
    obj->vertices.nextIndex,
    obj->texCoords.nextIndex,
    obj->normals.nextIndex,
    obj->faces.nextIndex,
    obj->groups.nextIndex);
  #endif

  free(line);
  fclose (obj_fp);

  return obj;
}

/**
  Freeing allocations & cleaning up after an obj.

  @param SimpleObj_t* obj A pointer to an obj
*/
void disposeObj(SimpleObj_t* obj) {
  objDataArrayDispose(&obj->vertices);
  objDataArrayDispose(&obj->texCoords);
  objDataArrayDispose(&obj->normals);
  int f;
  for (f = 0; f < obj->faces.nextIndex; f++) {
    objDataArrayDispose((ObjDataArray_t*)objDataArrayAccess(&obj->faces, f));
  }
  int g;
  for (g = 0; g < obj->groups.nextIndex; g++) {
    ObjGroup_t* group = objDataArrayAccess(&obj->groups, g);
    free(group->name);
  }
  objDataArrayDispose(&obj->groups);
  objDataArrayDispose(&obj->faces);
  // objDataArrayDispose(&obj->lines);
  free(obj);
}

#ifdef __GLUT_H__
/**
  Naively drawing a loaded obj.
  May work with more than libglut/libGL but is untested with anything else.

  @param SimpleObj_t* a pointer to an obj
*/
void drawObj(SimpleObj_t* obj) {
  int g;
  for (g = 0; g < obj->groups.nextIndex; g++) {
    ObjGroup_t* group = objDataArrayAccess(&obj->groups, g);
    if (!group->render) continue;
    int f;
    for (f = group->startFace; f < group->endFace; f++) {
      ObjDataArray_t* face = objDataArrayAccess(&obj->faces, f);
      glBegin(GL_POLYGON);
      int f_c;
      for (f_c = 0; f_c < face->nextIndex; f_c++) {
        ObjFaceComponent_t* faceComp = objDataArrayAccess(face, f_c);
        ObjVertex_t* vertex
          = objDataArrayAccess(&obj->vertices, faceComp->vertexIndex-1);
        if (faceComp->normalIndex > 0) {
          ObjVertexNormal_t* normal
            = objDataArrayAccess(&obj->normals, faceComp->normalIndex-1);
          glNormal3d(normal->x, normal->y, normal->z);
        }
        if (faceComp->texCoordIndex > 0) {
          ObjTexCoord_t* texCoord
            = objDataArrayAccess(&obj->texCoords, faceComp->texCoordIndex-1);
          glTexCoord3d(texCoord->u, texCoord->v, texCoord->w);
        }
        if (vertex->r > -1) {
          glColor3d(vertex->r, vertex->g, vertex->b);
        }
        glVertex4d(vertex->x, vertex->y, vertex->z, vertex->w);
      }
      glEnd();
    }
  }
}
#endif
#endif
