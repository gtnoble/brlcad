# MGED High-Level Lisp API Guide

This guide documents the high-level, idiomatic Lisp API for MGED. This API provides a more natural Lisp interface compared to the low-level command bindings.

## Table of Contents

- [Overview](#overview)
- [Getting Started](#getting-started)
- [Primitive Creation](#primitive-creation)
- [Display Operations](#display-operations)
- [Object Queries](#object-queries)
- [Combinations and Regions](#combinations-and-regions)
- [Object Manipulation](#object-manipulation)
- [Transformations](#transformations)
- [Complete Examples](#complete-examples)

## Overview

The MGED-API package provides idiomatic Lisp wrappers around MGED commands with:

- **Keyword arguments** instead of string flags
- **Vector representation** for points and directions: `#(x y z)`
- **Structured data** where possible
- **Better error handling** with Lisp conditions

### Package Structure

- **MGED package**: Low-level command bindings (e.g., `mged:ls`, `mged:draw`)
- **MGED-API package**: High-level API (e.g., `mged-api:make-sphere`)

## Getting Started

When using the ECL REPL in MGED, you can access both packages:

```lisp
;; Use low-level commands from MGED package
(mged:ls)

;; Use high-level API from MGED-API package
(mged-api:make-sphere "ball" #(0 0 0) 10)

;; For convenience, you can use the API package
(use-package :mged-api)
(make-sphere "ball2" #(10 0 0) 5)
```

## Primitive Creation

All primitive creation functions use vectors `#(x y z)` for points and directions.

### Spheres

```lisp
;; Create a sphere at origin with radius 10
(mged-api:make-sphere "ball" #(0 0 0) 10)

;; Create a sphere at a specific location
(mged-api:make-sphere "ball2" #(100 50 25) 15)
```

### Cylinders

```lisp
;; Create a cylinder (RCC - Right Circular Cylinder)
;; Base at origin, height vector, radius
(mged-api:make-cylinder "cyl1" 
                        #(0 0 0)      ; base center
                        #(0 0 100)    ; height vector
                        10)           ; radius

;; Horizontal cylinder
(mged-api:make-cylinder "cyl2"
                        #(0 0 0)
                        #(100 0 0)
                        5)
```

### Cones

```lisp
;; Create a truncated cone
(mged-api:make-cone "cone1"
                    #(0 0 0)        ; base center
                    #(0 0 50)       ; height vector
                    20              ; base radius
                    5)              ; top radius

;; Perfect cone (top radius = 0)
(mged-api:make-cone "cone2"
                    #(0 0 0)
                    #(0 0 100)
                    30
                    0)
```

### Boxes

```lisp
;; Create a box (RPP - Right Rectangular Parallelepiped)
(mged-api:make-box "box1"
                   #(0 0 0)         ; corner point
                   #(50 30 20))     ; dimensions (w h d)

;; Centered box
(mged-api:make-box "box2"
                   #(-25 -25 -25)
                   #(50 50 50))
```

### Torus

```lisp
;; Create a torus
(mged-api:make-torus "tor1"
                     #(0 0 0)       ; center
                     #(0 0 1)       ; normal vector
                     5              ; tube radius
                     20)            ; distance from center

;; Vertical torus
(mged-api:make-torus "tor2"
                     #(0 0 0)
                     #(1 0 0)
                     3
                     15)
```

### ARB8 (Arbitrary 8-Vertex Polyhedron)

```lisp
;; Create a custom 8-vertex shape
(mged-api:make-arb8 "wedge1"
  (list #(0 0 0)      ; vertex 1
        #(10 0 0)     ; vertex 2
        #(10 10 0)    ; vertex 3
        #(0 10 0)     ; vertex 4
        #(0 0 10)     ; vertex 5
        #(10 0 10)    ; vertex 6
        #(10 10 10)   ; vertex 7
        #(0 10 10)))  ; vertex 8
```

### Ellipsoid

```lisp
;; Create an ellipsoid with three semi-axes
(mged-api:make-ellipsoid "ell1"
                         #(0 0 0)       ; center
                         #(10 0 0)      ; semi-axis A
                         #(0 5 0)       ; semi-axis B
                         #(0 0 3))      ; semi-axis C
```

### Low-Level Primitive Creation

For other primitive types, use `create-primitive`:

```lisp
;; Generic primitive creation - automatically converts vectors
(mged-api:create-primitive "part1" "part"
                           #(0 0 0)     ; base
                           #(0 0 50)    ; height
                           10           ; base radius
                           0)           ; top radius (point)
```

## Display Operations

### Drawing Objects

```lisp
;; Draw a single object
(mged-api:draw-objects "ball")

;; Draw multiple objects
(mged-api:draw-objects '("ball" "cyl1" "box1"))

;; Draw with custom color (RGB vector)
(mged-api:draw-objects "ball" :color #(255 0 0))  ; red
(mged-api:draw-objects "cyl1" :color #(0 255 0))  ; green
(mged-api:draw-objects "box1" :color #(0 0 255))  ; blue

;; Draw shaded (requires OpenGL + zbuffer + lighting)
(mged-api:draw-objects "ball" :shaded t)

;; Draw without auto-resizing view
(mged-api:draw-objects "newobj" :no-resize t)

;; Simplified drawing (skip subtractions)
(mged-api:draw-objects "complex" :simplified t)

;; Set wireframe threshold for BOTs
(mged-api:draw-objects "mesh" :wireframe-threshold 10000)
```

### Erasing Objects

```lisp
;; Erase a single object
(mged-api:erase-objects "ball")

;; Erase multiple objects
(mged-api:erase-objects '("ball" "cyl1" "box1"))

;; Clear entire display
(mged-api:clear-display)
```

## Object Queries

### Listing Objects

The `list-objects` function returns different data structures depending on whether `:long-format` is used:

- **Without `:long-format`**: Returns a list of object name strings
- **With `:long-format`**: Returns a list of plists containing detailed object information

#### Basic Listing

```lisp
;; List all visible objects - returns list of names
(mged-api:list-objects)
;; => ("ball.s" "ball.s2" "cyl.s" "box.r")

;; List all objects (including hidden)
(mged-api:list-objects :all t)

;; List only primitives
(mged-api:list-objects :primitives t)
;; => ("ball.s" "ball.s2" "cyl.s")

;; List only regions
(mged-api:list-objects :regions t)
;; => ("box.r" "assembly.r")

;; List only combinations
(mged-api:list-objects :combinations t)

;; List objects matching a pattern
(mged-api:list-objects :pattern "ball*")
;; => ("ball.s" "ball.s2" "ball_copy.s")
```

#### Long Format Listing

When using `:long-format t`, each object is returned as a plist with the following keys:
- `:name` - Object name
- `:type` - Object type (e.g., "ell", "rcc", "arb8", "comb", "region")
- `:major-type` - Major type code
- `:minor-type` - Minor type code
- `:length` - Object size/length

```lisp
;; Long format - returns list of plists
(mged-api:list-objects :long-format t)
;; => ((:name "ball.s" :type "ell" :major-type "1" :minor-type "3" :length "120")
;;     (:name "cyl.s" :type "rcc" :major-type "1" :minor-type "4" :length "128"))

;; Long format with human-readable sizes
(mged-api:list-objects :long-format t :human-readable t)

;; Sort by size instead of name
(mged-api:list-objects :long-format t :sort-by-size t)
```

#### Working with Results

```lisp
;; Iterate over object names
(dolist (name (mged-api:list-objects))
  (format t "Object: ~A~%" name))

;; Filter objects
(let ((objects (mged-api:list-objects)))
  (remove-if-not #'(lambda (name) (search "ball" name))
                 objects))

;; Process long format data
(let ((objects (mged-api:list-objects :long-format t)))
  (dolist (obj objects)
    (format t "~A is a ~A (~A bytes)~%"
            (getf obj :name)
            (getf obj :type)
            (getf obj :length))))

;; Find all primitives of a specific type
(let ((objects (mged-api:list-objects :primitives t :long-format t)))
  (remove-if-not #'(lambda (obj) (string= (getf obj :type) "ell"))
                 objects))
;; => List of all ellipsoid primitives

;; Get total object count
(length (mged-api:list-objects))

;; Check if any objects match a pattern
(when (mged-api:list-objects :pattern "temp*")
  (format t "Found temporary objects~%"))
```

### Checking Existence

```lisp
;; Check if object exists
(if (mged-api:object-exists-p "ball")
    (format t "Ball exists~%")
    (format t "Ball not found~%"))
```

### Getting Object Information

```lisp
;; Get object info
(mged-api:get-object-info "ball")

;; Get object attributes
(mged-api:get-object-info "region1" :attributes t)
```

## Combinations and Regions

### Creating Combinations

```lisp
;; Simple combination (union)
(mged-api:make-combination "assembly1"
  '("ball" "cyl1" "box1"))

;; Combination with operations
(mged-api:make-combination "part1"
  '(("ball" :union)
    ("cyl1" :subtract)
    ("box1" :intersect)))

;; Combination with color
(mged-api:make-combination "assembly2"
  '("ball" "cyl1")
  :color #(128 128 255))

;; Combination with shader
(mged-api:make-combination "shiny"
  '("ball")
  :shader "plastic")
```

### Creating Regions

```lisp
;; Simple region
(mged-api:make-region "region1"
  '(("ball" :union)
    ("cyl1" :subtract)))

;; Region with ID and color
(mged-api:make-region "region2"
  '("box1" "tor1")
  :id 1000
  :color #(200 100 50))

;; Region with shader and material
(mged-api:make-region "metal_part"
  '(("base" :union)
    ("hole" :subtract))
  :id 2000
  :color #(192 192 192)
  :shader "plastic"
  :material "steel")
```

### Creating Groups

```lisp
;; Group (union of all members)
(mged-api:make-group "wheel_assembly"
  '("hub" "rim" "tire" "spokes"))
```

## Object Manipulation

### Copying Objects

```lisp
;; Copy an object
(mged-api:copy-object "ball" "ball_copy")
```

### Moving/Renaming Objects

```lisp
;; Rename an object
(mged-api:move-object "old_name" "new_name")
```

### Deleting Objects

The MGED API provides three high-level functions for deleting objects with various options and safety features.

#### Basic Object Deletion

```lisp
;; Delete specific objects
(mged-api:kill-objects '("sphere1" "box2"))

;; Delete a single object
(mged-api:kill-objects "temp_obj")

;; Delete with force flag (no complaints about missing objects)
(mged-api:kill-objects "maybe_missing" :force t)

;; Delete quietly (suppress lookup failure messages)
(mged-api:kill-objects '("obj1" "obj2") :quiet t)
```

#### Delete All Objects and References

```lisp
;; Delete specific objects and all references to them
(mged-api:kill-all-objects '("sphere1" "box2"))

;; Delete ALL objects in database (use with extreme caution!)
(mged-api:kill-all-objects)

;; Dry run to see what would be deleted
(mged-api:kill-all-objects :dry-run t)
;; => ("sphere1" "box2" "assembly1" "component1")

;; Dry run for specific objects
(mged-api:kill-all-objects '("temp1" "temp2") :dry-run t)
;; => ("temp1" "temp2" "ref1" "ref2")
```

#### Recursive Tree Deletion

```lisp
;; Delete object tree recursively
(mged-api:kill-object-tree "assembly1")

;; Delete with all references (safer than :force)
(mged-api:kill-object-tree '("group1" "group2") :all t)

;; Force delete (may create dangling references)
(mged-api:kill-object-tree "complex_assembly" :force t)

;; Dry run to see what would be deleted
(mged-api:kill-object-tree "assembly1" :dry-run t)
;; => ("assembly1" "component1" "component2" "subpart1" "subpart2")
```

#### Safety Features

**Dry Run Mode**: All three functions support `:dry-run t` to preview what would be deleted:

```lisp
;; Safe pattern: always dry run first
(let ((to-delete (mged-api:kill-all-objects "temp*" :dry-run t)))
  (when (and to-delete 
             (y-or-n-p "Delete ~D objects? ~{~A ~}" (length to-delete) to-delete))
    (mged-api:kill-all-objects "temp*")))
```

**Force vs Quiet Options**:
- `:force t` - Don't complain if objects don't exist
- `:quiet t` - Suppress database lookup failure messages
- `:all t` - Kill objects and clean up all references (safer than `:force`)

**Warning**: All delete operations are destructive and cannot be undone. Always consider using `:dry-run t` first to verify what will be deleted.

## Transformations

### Translation

```lisp
;; Translate by offset vector
(mged-api:translate-object "ball" #(10 20 30))
```

### Rotation

```lisp
;; Rotate about current center
(mged-api:rotate-object "box1" #(45 0 0))  ; 45° about X

;; Rotate about specific point
(mged-api:rotate-object "cyl1"
                        #(0 90 0)      ; 90° about Y
                        :about-point #(50 50 50))
```

### Scaling

```lisp
;; Uniform scaling
(mged-api:scale-object "ball" 2.0 :uniform t)

;; Non-uniform scaling (stretch)
(mged-api:scale-object "box1" #(2.0 1.0 0.5))
```

## Raytracing

The raytracing functions provide high-level wrappers around BRL-CAD's `rt` command for rendering scenes.

### Basic Raytracing

```lisp
;; Quick preview on framebuffer
(mged-api:quick-preview '("ball" "cyl1"))

;; Custom preview with specific framebuffer
(mged-api:quick-preview nil :framebuffer "/dev/ogl")

;; High-quality render to file
(mged-api:high-quality-render '("scene") "output.png")

;; Custom quality settings
(mged-api:high-quality-render '("model")
                              "render.png"
                              :width 3840
                              :height 2160
                              :hypersample 8)
```

### Advanced Raytracing Options

```lisp
;; Full control with raytrace function
(mged-api:raytrace '("assembly")
                   :size 1024
                   :output-file "image.png"
                   :background-color #(135 206 235)  ; sky blue
                   :perspective 30
                   :hypersample 4
                   :jitter 1
                   :processors 8)

;; Render with custom lighting
(mged-api:raytrace nil  ; uses displayed objects
                   :width 1920
                   :height 1080
                   :lighting-model 0
                   :ambient-light 0.4
                   :output-file "lit_scene.png")

;; Benchmark render (reproducible results)
(mged-api:raytrace '("benchmark_model")
                   :size 512
                   :benchmark t
                   :output-file "bench.pix")

;; White background render
(mged-api:raytrace '("product")
                   :size 2048
                   :background-color #(255 255 254)
                   :output-file "product.png")

;; Render with gamma correction
(mged-api:raytrace '("scene")
                   :width 1440
                   :height 972
                   :gamma 2.2
                   :jitter 1
                   :output-file "corrected.png")

;; Different lighting models
(mged-api:raytrace '("model")
                   :size 512
                   :lighting-model 2  ; show normals as colors
                   :framebuffer "/dev/Xl")
```

### Raytracing Parameters

**Output Control:**
- `:size` - Square image size (default 512)
- `:width` / `:height` - Specific dimensions (override :size)
- `:output-file` - Path for output (.pix or .png)
- `:framebuffer` - Framebuffer device (e.g., "/dev/Xl")

**View Control:**
- `:background-color` - RGB vector #(r g b), values 0-255
- `:perspective` - Perspective angle in degrees (0-180)
- `:azimuth` / `:elevation` - View angles (for auto-sizing)
- `:aspect-ratio` - Number or ratio string (e.g., "16:9")

**Quality Control:**
- `:lighting-model` - 0=full (default), 1=diffuse, 2=normals, 3-7=various
- `:ambient-light` - 0.0-1.0 ambient intensity
- `:hypersample` - Extra rays per pixel for antialiasing
- `:jitter` - 1=randomize rays, 2=frame shift, 3=both
- `:processors` - Max processors to use

**Other Options:**
- `:benchmark` - Disable random effects for reproducibility
- `:report-overlaps` - Report overlapping regions (default T)
- `:gamma` - Gamma correction value (e.g., 2.2)
- `:use-air` - Enable air region rendering
- `:additional-options` - List of extra rt flags

## Complete Examples

### Example 1: Simple Tank Turret

```lisp
(use-package :mged-api)

;; Create primitives
(make-cylinder "barrel" #(0 0 50) #(100 0 0) 5)
(make-sphere "turret" #(0 0 50) 30)
(make-cylinder "base" #(0 0 0) #(0 0 50) 40)

;; Create the assembly
(make-combination "tank_turret"
  '(("base" :union)
    ("turret" :union)
    ("barrel" :union))
  :color #(100 100 100))

;; Draw it
(draw-objects "tank_turret" :color #(128 128 128))
```

### Example 2: Bolt with Threads

```lisp
(use-package :mged-api)

;; Head
(make-cylinder "head" #(0 0 0) #(0 0 10) 15)

;; Shaft
(make-cylinder "shaft" #(0 0 10) #(0 0 50) 8)

;; Create region
(make-region "bolt"
  '("head" "shaft")
  :id 1001
  :color #(192 192 192)
  :material "steel")

;; Draw
(draw-objects "bolt")
```

### Example 3: House Frame

```lisp
(use-package :mged-api)

;; Floor
(make-box "floor" #(-50 -50 0) #(100 100 5))

;; Walls
(make-box "wall_north" #(-50 45 5) #(100 5 30))
(make-box "wall_south" #(-50 -50 5) #(100 5 30))
(make-box "wall_east" #(45 -50 5) #(5 100 30))
(make-box "wall_west" #(-50 -50 5) #(5 100 30))

;; Roof
(make-box "roof" #(-55 -55 35) #(110 110 3))

;; Door opening (to subtract)
(make-box "door" #(-5 -50 5) #(10 5 20))

;; Create house region
(make-region "house"
  '(("floor" :union)
    ("wall_north" :union)
    ("wall_south" :union)
    ("wall_east" :union)
    ("wall_west" :union)
    ("roof" :union)
    ("door" :subtract))
  :id 2001
  :color #(200 150 100))

;; Draw the house
(draw-objects "house")
```

### Example 4: Parametric Design

```lisp
(use-package :mged-api)

;; Function to create a parametric wheel
(defun make-wheel (name center radius thickness spoke-count)
  "Create a wheel with spokes."
  (let ((rim-name (format nil "~A_rim" name))
        (hub-name (format nil "~A_hub" name))
        (spoke-names '()))
    
    ;; Create rim (torus)
    (make-torus rim-name
                center
                #(0 0 1)
                (/ thickness 2)
                radius)
    
    ;; Create hub (cylinder)
    (make-cylinder hub-name
                   (vector (aref center 0)
                          (aref center 1)
                          (- (aref center 2) (/ thickness 2)))
                   #(0 0 0) (vector 0 0 thickness)
                   (/ radius 4))
    
    ;; Create spokes
    (dotimes (i spoke-count)
      (let* ((angle (* 2 pi (/ i spoke-count)))
             (spoke-name (format nil "~A_spoke~D" name i))
             (x (* radius 0.8 (cos angle)))
             (y (* radius 0.8 (sin angle))))
        (make-cylinder spoke-name
                      (vector (+ (aref center 0) (* x 0.3))
                             (+ (aref center 1) (* y 0.3))
                             (- (aref center 2) (/ thickness 2)))
                      (vector 0 0 thickness)
                      (/ radius 20))
        (push spoke-name spoke-names)))
    
    ;; Create combination
    (make-combination name
                     (cons rim-name (cons hub-name spoke-names))
                     :color #(128 128 128))
    
    name))

;; Use the function
(make-wheel "front_wheel" #(0 0 0) 30 10 8)
(draw-objects "front_wheel")
```

## Tips and Best Practices

1. **Use vectors for geometry**: Always use `#(x y z)` for points and directions
2. **Keyword arguments**: Take advantage of keywords for cleaner, more readable code
3. **Check existence**: Use `object-exists-p` before operations that might fail
4. **Error handling**: Wrap operations in `handler-case` for robust code
5. **Parametric design**: Use Lisp functions to create reusable design patterns
6. **Package usage**: Either qualify names (`mged-api:make-sphere`) or `use-package`

## See Also

- MGED Command Reference: https://brlcad.github.io/docs/man/n.html
- ECL Documentation: https://ecl.common-lisp.dev/
- Common Lisp Reference: http://www.lispworks.com/documentation/HyperSpec/
