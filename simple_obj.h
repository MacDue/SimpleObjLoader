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

/* Simple Obj Loader

   v0.1.2
   Author: MacDue
   License: Unlicense

   Loading and drawing Wavefront .obj files.
   Supports a subset of the full .obj formal.

   - Vertices (v)
   - Vertex normals (vn)
   - Texture coordinates (vt)
   - Faces (f)
   - Simple uncached drawing
*/

typedef struct DataArray {
  void* data;
  int len;
  int nextIndex;
  int typeSize;
} DataArray_t;

typedef struct ObjFaceComponent {
  int vertexIndex;
  int texCoordIndex;
  int normalIndex;
} ObjFaceComponent_t;

typedef struct ObjVertex {
  double x, y, z, w;
} ObjVertex_t;

typedef struct ObjTexCoord {
  double u, v, w;
} ObjTexCoord_t;

typedef struct ObjVertexNormal {
  double x, y, z;
} ObjVertexNormal_t;

typedef struct SimpleObj {
  DataArray_t/*<ObjVertex_t>*/ vertices;
  DataArray_t/*<ObjTexCoord_t>*/ texCoords;
  DataArray_t/*<ObjVertexNormal_t>*/ normals;
  DataArray_t/*<DataArray_t<ObjFaceComponent_t>*/ faces;
  DataArray_t/*<int>*/ lines;
} SimpleObj_t;

SimpleObj_t* loadObj(char* file_name);
void disposeObj(SimpleObj_t* obj);
void drawObj(SimpleObj_t* obj);

// Will write to returnArray.
#define STANDARD_ARRAY_PARSER(arrayType_t, strtoFunc)             \
static int                                                        \
parseArray_ ## arrayType_t                                        \
(char* arrayStr, int minSize, arrayType_t* returnArray) {         \
  replaceDelims(arrayStr);                                        \
  DataArray_t doubleData = {};                                    \
  initDataArray(                                                  \
    &doubleData, sizeof (arrayType_t), minSize, returnArray);     \
  char* next = NULL;                                              \
  char* previous = arrayStr;                                      \
  for (;;) {                                                      \
    arrayType_t d = strtoFunc;                                    \
    if (next == previous) {                                       \
      break;                                                      \
    }                                                             \
    previous = next;                                              \
    dataArrayAppend(&doubleData,&d);                              \
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

static inline void* dataArrayAccess(DataArray_t* dataArray, int index) {
  return (void*)((size_t)dataArray->data+index*dataArray->typeSize);
}

static DataArray_t*
dataArrayAppend (DataArray_t* dataArray, void* data) {
  // Type safey is for the weak.
  if (dataArray->nextIndex >= dataArray->len) {
    int newLen = dataArray->len * 2;
    dataArray->data = realloc(dataArray->data, newLen * dataArray->typeSize);
    dataArray->len = newLen;
  }
  // Must directly copy memory to avoid any casting.
  memcpy (
    dataArrayAccess(dataArray, dataArray->nextIndex),
    data, dataArray->typeSize
  );
  dataArray->nextIndex++;
  return dataArray;
}

static inline void dataArrayDispose(DataArray_t* dataArray) {
  free(dataArray->data);
}

static DataArray_t*
initDataArray (DataArray_t* dataArray, int typeSize, int reserve, void* array) {
  dataArray->data = array == NULL ? malloc(typeSize * reserve) : array;
  dataArray->len = reserve;
  dataArray->nextIndex = 0;
  dataArray->typeSize = typeSize;
  return dataArray;
}

// Create some generic parsers
STANDARD_ARRAY_PARSER(double, strtod(previous, &next))
STANDARD_ARRAY_PARSER(long, strtol(previous, &next, 10))

static SimpleObj_t* newSimpleObj(void) {
  SimpleObj_t* obj = malloc(sizeof (SimpleObj_t));
  initDataArray(&obj->vertices, sizeof (ObjVertex_t), 100, NULL);
  initDataArray(&obj->texCoords, sizeof (ObjTexCoord_t), 100, NULL);
  initDataArray(&obj->normals, sizeof (ObjVertexNormal_t), 100, NULL);
  initDataArray(&obj->faces, sizeof (DataArray_t), 100, NULL);
  initDataArray(&obj->lines, sizeof (int*), 100, NULL);
  return obj;
}

static long longDataBuffer[4];
static double dataInputBuffer[4];

/*
  Load a Wavefront .obj file.

  @param char* a path to a .obj
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
  size_t lineLen;
  char* line = NULL;
  size_t len = 0;

  while ((lineLen = getline(&line, &len, obj_fp)) != -1) {
    char* lineType = strtok(line, " ");
    char* remaining = strtok(NULL, "\n");
    if (lineType == NULL || remaining == NULL) {
      /* Junk */
      continue;
    }
    if (strcmp(lineType, OBJ_COMMENT) == 0) {
      /* Comments */
      continue;
    } else if (strcmp(lineType, OBJ_VERTEX) == 0) {
      /* Vertices */
      int vertexLen = parseArray_double(remaining, 4, dataInputBuffer);
      assert(vertexLen == 3 || vertexLen == 4);
      ObjVertex_t vertex = { .x = dataInputBuffer[0],
                             .y = dataInputBuffer[1],
                             .z = dataInputBuffer[2],
                             .w = vertexLen == 4 ? dataInputBuffer[3] : 1 };
      dataArrayAppend(&obj->vertices, &vertex);
    } else if (strcmp(lineType, OBJ_VERTEX_NORMAL) == 0) {
      /* Normals */
      int normalLen = parseArray_double(remaining, 3, dataInputBuffer);
      assert(normalLen == 3);
      ObjVertexNormal_t normal = { .x = dataInputBuffer[0],
                                   .y = dataInputBuffer[1],
                                   .z = dataInputBuffer[2] };
      dataArrayAppend(&obj->normals, &normal);
    } else if (strcmp(lineType, OBJ_VERTEX_TEX_CORD) == 0) {
      /* Texture coordinates */
      int textCoordLen = parseArray_double(remaining, 3, dataInputBuffer);
      assert(textCoordLen == 2 || textCoordLen == 3);
      ObjTexCoord_t texCoord = { .u = dataInputBuffer[0],
                                 .v = dataInputBuffer[1],
                                 .w = textCoordLen > 2
                                        ? dataInputBuffer[2] : 0};
      dataArrayAppend(&obj->texCoords, &texCoord);
    } else if (strcmp(lineType, OBJ_FACE) == 0) {
      /* Faces */
      DataArray_t face = {};
      // Most faces seem to have at most 4 vertices.
      initDataArray(&face, sizeof(ObjFaceComponent_t), 4, NULL);
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
        dataArrayAppend(&face, &faceComponent);
        faceComponentStr = strtok(NULL, " ");
      }
      dataArrayAppend(&obj->faces, &face);
    } else {
      #ifdef DEBUG
      fprintf(stderr, "Unknown line type '%s'\n", lineType);
      #endif
    }
  }

  free(line);
  fclose (obj_fp);

  #ifdef DEBUG
  printf("Loaded obj: vertices %d, texture coords %d, normals %d, faces %d\n",
          obj->vertices.nextIndex,
          obj->texCoords.nextIndex,
          obj->normals.nextIndex,
          obj->faces.nextIndex);
  #endif

  return obj;
}

/**
  Freeing allocations & cleaning up after an obj.

  @param SimpleObj_t* a pointer to an obj
*/
void disposeObj(SimpleObj_t* obj) {
  dataArrayDispose(&obj->vertices);
  dataArrayDispose(&obj->texCoords);
  dataArrayDispose(&obj->normals);
  int f;
  for (f = 0; f < obj->faces.nextIndex; f++) {
    dataArrayDispose((DataArray_t*)dataArrayAccess(&obj->faces, f));
  }
  dataArrayDispose(&obj->faces);
  dataArrayDispose(&obj->lines);
  free(obj);
}

#ifdef __GLUT_H__

/**
  Naively drawing a loaded obj.
  Probably works with more than libglut/libGL but is untested with anything.

  @param SimpleObj_t* a pointer to an obj
*/
void drawObj(SimpleObj_t* obj) {
  int f;
  for (f = 0; f < obj->faces.nextIndex; f++) {
    DataArray_t* face = dataArrayAccess(&obj->faces, f);
    glBegin(GL_POLYGON);
    int f_c;
    for (f_c = 0; f_c < face->nextIndex; f_c++) {
      ObjFaceComponent_t* faceComp = dataArrayAccess(face, f_c);
      ObjVertex_t* vertex
        = dataArrayAccess(&obj->vertices, faceComp->vertexIndex-1);
      if (faceComp->normalIndex > 0) {
        ObjVertexNormal_t* normal
          = dataArrayAccess(&obj->normals, faceComp->normalIndex-1);
        glNormal3d(normal->x, normal->y, normal->z);
      }
      if (faceComp->texCoordIndex > 0) {
        ObjTexCoord_t* texCoord
          = dataArrayAccess(&obj->texCoords, faceComp->texCoordIndex-1);
        glTexCoord3d(texCoord->u, texCoord->v, texCoord->w);
      }
      glVertex4d(vertex->x, vertex->y, vertex->z, vertex->w);
    }
    glEnd();
  }
}

#endif

#endif
