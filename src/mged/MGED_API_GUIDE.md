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

The MGED API now provides intuitive boolean expression syntax that eliminates confusion about operation precedence. Instead of understanding BRL-CAD's complex order of operations, you write standard Lisp expressions with clear parentheses.

### Boolean Expression Syntax

**Operators** (symbols):
- `union` - Boolean union (equivalent to `u` operator)
- `subtract` - Boolean subtraction (equivalent to `-` operator)  
- `intersect` - Boolean intersection (equivalent to `+` operator)

**Operands**: Strings containing object names

### Creating Combinations

```lisp
;; Simple union
(mged-api:make-combination "assembly1" 
  '(union "ball" "cyl1" "box1"))

;; Nested expressions with clear precedence
(mged-api:make-combination "machined_part" 
  '(union "base_piece" 
          (subtract "raw_piece" "cutter")
          (intersect "hole1" "hole2")))

;; Complex nested expression
(mged-api:make-combination "complex_assembly"
  '(union "sphere1" 
          (subtract "sphere2" 
                   (intersect "sphere3" "sphere4"))
          "sphere5"))

;; Multi-operand union
(mged-api:make-combination "many_parts"
  '(union "part1" "part2" "part3" "part4" "part5"))

;; With color and shader
(mged-api:make-combination "colored_assembly"
  '(union "base" (subtract "body" "hole"))
  :color #(128 128 255)
  :shader "plastic")
```

### Creating Regions

```lisp
;; Simple region with subtraction
(mged-api:make-region "region1"
  '(union "sphere1" (subtract "sphere2" "cube1"))
  :id 100)

;; Complex region with multiple operations
(mged-api:make-region "steel_part"
  '(union "base" (subtract "raw" "cutter"))
  :id 200
  :color #(192 192 192)
  :material "steel")

;; Region with nested intersections
(mged-api:make-region "precision_part"
  '(union "main_body" 
          (subtract "blank" 
                   (intersect "drill_hole1" "drill_hole2")))
  :id 300
  :color #(150 100 200)
  :attributes '(("part_number" . "A-123")
                ("revision" . "v2")
                ("surface_finish" . "polished")))

;; Multi-operand region
(mged-api:make-region "assembly_region"
  '(union "component1" "component2" "component3")
  :id 400
  :color #(100 200 100))
```

### Creating Groups

Groups are simplified unions for the most common case:

```lisp
;; Simple group (union of all members)
(mged-api:make-group "wheel_assembly"
  '("hub" "rim" "tire" "spokes"))

;; Group with color
(mged-api:make-group "red_parts"
  '("sphere1" "cylinder1" "box1")
  :color #(255 0 0))

;; Group with shader
(mged-api:make-group "shiny_parts"
  '("ball" "torus")
  :shader "mirror")
```

### Expression Examples

The power of the new syntax is clear with complex expressions:

```lisp
;; Before (confusing precedence):
;; (make-combination "old" '((a . :union) (b . :subtract) (c . :intersect)))
;; Results in: a u b - c  (unclear precedence)

;; After (clear intent):
(mged-api:make-combination "new" 
  '(union "a" (subtract "b" (intersect "c"))))
;; Results in: (a u (b - (c + d)))  (clear parenthesized expression)

;; Real-world complex example
(mged-api:make-combination "engine_block"
  '(union "block_base" 
          (subtract "cylinder_banks" 
                   (union "cylinder1_hole" "cylinder2_hole" "cylinder3_hole" "cylinder4_hole"))
          (intersect "coolant_passage1" "coolant_passage2"))
  :color #(128 128 128)
  :material "aluminum")
```

### Expression Validation

The API validates expressions and provides clear error messages:

```lisp
;; Valid expressions (no errors)
(mged-api:make-combination "valid1" '(union "sphere1" "sphere2"))
(mged-api:make-combination "valid2" '(union "a" (subtract "b" (intersect "c" "d"))))

;; Invalid expressions (produce helpful errors)
;; (make-combination "bad1" '("union" "sphere1" "sphere2"))
;; Error: Boolean operators must be symbols, got: STRING

;; (make-combination "bad2" '(xor "sphere1" "sphere2"))  
;; Error: Unknown boolean operation: XOR. Use: union, subtract, or intersect

;; (make-combination "bad3" '(union "sphere1"))
;; Error: Operation UNION requires at least 2 operands, got: 1

;; (make-combination "bad4" '(union "" "sphere2"))
;; Error: Object names must be non-empty strings, got: ""
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

## Attribute Management

The MGED API provides comprehensive attribute management functions that offer structured, idiomatic Lisp interfaces to the `attr` command. Attributes are key-value pairs that can be attached to database objects for metadata, material properties, and other information.

### Getting Attributes

#### Basic Attribute Retrieval

```lisp
;; Get all attributes for a single object
(get-attributes "region1")
;; => (:MATERIAL-ID "10" :REGION "R" :LOS "100")

;; Get specific attributes
(get-attributes "region1" :attribute-names '("material_id" "color"))
;; => (:MATERIAL-ID "10" :COLOR "255/0/0")

;; Get attributes from multiple objects
(get-attributes "region*")
;; => (("region1" (:MATERIAL-ID "10" :REGION "R"))
;;     ("region2" (:MATERIAL-ID "20" :REGION "R")))
```

#### Working with Multiple Objects

```lisp
;; Process attributes from multiple objects
(let ((objects-with-attrs (get-attributes "region*")))
  (dolist (obj-info objects-with-attrs)
    (let ((obj-name (car obj-info))
          (attrs (cdr obj-info)))
      (format t "~A: ~A~%" obj-name (getf attrs :material-id)))))

;; Find objects with specific attribute values
(let ((all-regions (get-attributes "region*")))
  (remove-if-not (lambda (obj-info)
                   (string= (getf (cdr obj-info) :region) "R"))
                 all-regions))
```

### Setting Attributes

#### Attribute Format (Alist Only)

The MGED API now supports **only alist format** for attribute specifications. An alist (association list) is a list of cons pairs where each pair is `(attribute-name . value)`.

```lisp
;; Correct: Using alist (association list)
(set-attributes "region1" '(("material_id" . "10") ("color" . "255/0/0")))

;; Set attributes on multiple objects
(set-attributes "region*" '(("region" . "R") ("los" . "100")))

;; Complex attribute sets
(set-attributes "complex_part" 
               '(("part_number" . "A-123")
                 ("revision" . "v2.1")
                 ("material" . "titanium")
                 ("weight_class" . "light")
                 ("inspection_required" . "true")))
```

**Note**: Previous support for plist and list-of-pairs formats has been removed for API consistency. All attribute functions now require alist format.

#### Integration with Object Creation

```lisp
;; Create a region with attributes
(make-region "part1" '("sphere" "cylinder") 
             :id 1001 
             :color #(200 100 50))

;; Add additional attributes
(set-attributes "part1" '(("part_number" . "A-123") 
                          ("revision" . "v2")
                          ("description" . "Main assembly part")))
```

### Removing Attributes

```lisp
;; Remove single attribute
(remove-attributes "region1" "temp_attr")

;; Remove multiple attributes
(remove-attributes "region*" '("temp_attr" "old_attr"))

;; Clean up temporary attributes from multiple objects
(let ((temp-attrs '("temp_flag" "debug_info" "test_attr")))
  (remove-attributes "temp_*" temp-attrs))
```

### Appending Attributes

```lisp
;; Append attributes (creates if doesn't exist)
(append-attributes "region1" '(("comment" . "Modified part") 
                               ("version" . "2")))

;; Add history tracking
(append-attributes "assembly1" '(("modified_by" . "designer1")
                                  ("modified_date" . "2023-10-20")))
```

### Listing Attribute Types

```lisp
;; List all attribute types in database
(list-attribute-types "*")
;; => ("material_id" "region" "los" "color" "shader" "region_id")

;; List attributes matching pattern
(list-attribute-types "*" :key-filter "material_*")
;; => ("material_id" "material_name")

;; List specific attribute values
(list-attribute-types "*" :key-filter "material_id" :value-filter "*")
;; => ("material_id=1" "material_id=2" "material_id=10")

;; Get all unique values for an attribute
(let ((material-values (list-attribute-types "*" :key-filter "material_id" :value-filter "*")))
  (mapcar (lambda (item)
            (subseq item (1+ (position #\= item))))
          material-values))
;; => ("1" "2" "10")
```

### Displaying and Sorting Attributes

```lisp
;; Pretty-print all attributes for an object
(show-attributes "region1")

;; Show specific attributes
(show-attributes "region*" :attribute-names '("material_id" "color"))

;; Sort attributes alphabetically (case-sensitive)
(sort-attributes "region1")

;; Sort attributes alphabetically (case-insensitive)
(sort-attributes "region*" :sort-type :nocase)

;; Sort by attribute values
(sort-attributes "region1" :sort-type :value)
```

### Copying Attributes

```lisp
;; Copy attribute between objects
(copy-attribute "region1" "material_id" "region2" "material_id")

;; Copy attribute to new attribute name (rename)
(copy-attribute "region1" "old_id" "region1" "new_id")

;; Batch copy attributes between similar objects
(defun copy-template-attributes (template target)
  "Copy common template attributes to target object"
  (dolist (attr '("material_id" "color" "shader"))
    (when (get-attributes template :attribute-names (list attr))
      (copy-attribute template attr target attr))))

(copy-template-attributes "template_region" "new_region")
```

### Advanced Attribute Operations

#### Attribute Validation

```lisp
(defun validate-region-attributes (region-name)
  "Validate that a region has required attributes"
  (let ((attrs (get-attributes region-name))
        (required '(:region :region-id :material-id)))
    (every (lambda (req-attr)
              (getf attrs req-attr))
            required)))

(validate-region-attributes "region1")
;; => T or NIL
```

#### Attribute Migration

```lisp
(defun migrate-attribute-names (object-pattern old->new-map)
  "Rename attributes according to mapping"
  (let ((objects (get-attributes object-pattern)))
    (dolist (obj-info objects)
      (let ((obj-name (car obj-info))
            (attrs (cdr obj-info)))
        (dolist (mapping old->new-map)
          (let ((old-name (car mapping))
                (new-name (cdr mapping)))
            (when (getf attrs (intern (string-upcase old-name) :keyword))
              (copy-attribute obj-name old-name obj-name new-name)
              (remove-attributes obj-name old-name)))))))

;; Rename attributes from old naming convention
(migrate-attribute-names "region*" 
                         '(("material_id" . "mat_id")
                           ("region_id" . "reg_id")))
```

#### Attribute Templates

```lisp
(defun apply-attribute-template (object-pattern template)
  "Apply a template of attributes to objects matching pattern"
  (set-attributes object-pattern template))

;; Define standard templates
(defparameter *steel-region-template*
  '(("material" . "steel")
    ("density" . "7850")
    ("surface_finish" . "machined")
    ("corrosion_resistance" . "high")))

(defparameter *aluminum-region-template*
  '(("material" . "aluminum")
    ("density" . "2700")
    ("surface_finish" . "anodized")
    ("corrosion_resistance" . "medium")))

;; Apply templates
(apply-attribute-template "steel_*" *steel-region-template*)
(apply-attribute-template "alum_*" *aluminum-region-template*)
```

### Integration with Existing Functions

#### Enhanced Object Information

```lisp
;; Get structured attribute information
(get-object-info "region1" :attributes t)
;; => (:MATERIAL-ID "10" :REGION "R" :LOS "100")

;; Combine with regular object info
(let ((basic-info (get-object-info "region1"))
      (attrs (get-object-info "region1" :attributes t)))
  (format t "Object: ~A~%" basic-info)
  (format t "Material: ~A~%" (getf attrs :material-id)))
```

#### Enhanced Region Creation

```lisp
;; Create region with comprehensive attributes
(make-region "complex_part" 
             '("base" "hole1" "hole2")
             :id 2001
             :color #(192 192 192)
             :attributes '(("part_number" . "CP-2001")
                          ("revision" . "v3.2")
                          ("material" . "titanium")
                          ("weight_class" . "light")
                          ("inspection_required" . "true")))
```

### Error Handling

```lisp
;; Safe attribute operations
(handler-case
    (get-attributes "nonexistent_object")
  (mged-error (e)
    (format t "Object not found: ~A~%" e)))

;; Validate attribute operations
(defun safe-set-attributes (object-pattern attributes)
  "Set attributes with error handling"
  (handler-case
      (progn
        (set-attributes object-pattern attributes)
        t)
    (mged-error (e)
      (format t "Failed to set attributes on ~A: ~A~%" object-pattern e)
      nil)))
```

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
  '(union "base" "turret" "barrel")
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
  '(union "floor" "wall_north" "wall_south" "wall_east" "wall_west" "roof"
          (subtract "door"))
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
