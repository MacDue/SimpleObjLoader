####  A really simple single header .obj with glut support.

Including:
```c
// Only needed at first include
#define SIMPLE_OBJ_IMP
// glut needs to be included first for drawObj implementation
#include <GL/glut.h>
#include "simple_obj.h"
```


Usage:

```c
// Loading an .obj
// 256 is the line input buffer size
SimpleObj_t* myObj = loadObj("some_model.obj", 256);
```

Drawing:
```c
// GL setup stuff, transforms, ect...
drawObj(myObj);
// More gl stuff
```

Working:

- Vertices
- Normals
- Texture coords
- Faces
- Basic rendering

Todo:
- Separate group rendering
- Materials
- Maybe support some more obj features
- Clean up and free stuff
