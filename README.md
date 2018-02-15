### Simple Obj Loader
####  A really simple single header Wavefront .obj loader with glut support.

Including:
```c
// Only needed at first include
#define SIMPLE_OBJ_IMP
// glut needs to be included first for drawObj implementation
#include <GL/glut.h>
#include "simple_obj.h"
```

##### Full docs:
Usage:

```c
// Loading a .obj
SimpleObj_t* myObj = loadObj("some_model.obj");
```

Drawing:
```c
// GL setup stuff, transforms, ect...
drawObj(myObj);
// More GL stuff
```

Freeing obj data:
```c
// This obj is trash
disposeObj(myObj);
// It's where it belongs now
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
- ~~Clean up and free stuff~~ (done)

##### Example result:

![alt text](https://i.imgur.com/ozT39Jh.png "Future gun")
