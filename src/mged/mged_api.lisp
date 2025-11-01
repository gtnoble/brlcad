;;;; MGED High-Level API 🖥️
;;;;
;;;; This file provides idiomatic Lisp wrappers around the low-level MGED command
;;;; bindings. It offers keyword arguments, vector representations for points/directions,
;;;; and better error handling compared to the raw command interface.
;;;;
;;;; This file is compiled by ECL at build time and linked into the mged binary.

(in-package :mged-api)

;;;; ============================================================================
;;;; Utility Functions
;;;; ============================================================================

(defun vector->string (vec)
  "Convert a vector #(x y z) to a space-separated string 'x y z'."
  (format nil "~{~A~^ ~}" (coerce vec 'list)))

(defun color->string (color)
  "Convert a color vector #(r g b) to a string 'r/g/b'."
  (etypecase color
    (vector (format nil "~D/~D/~D" 
                    (aref color 0) (aref color 1) (aref color 2)))
    (list (format nil "~D/~D/~D" 
                  (first color) (second color) (third color)))))

(defun ensure-list (obj)
  "Ensure OBJ is a list. If it's a single item, wrap it in a list."
  (if (listp obj) obj (list obj)))

(defun split-lines (string)
  "Split a string by newlines, removing empty lines.
   Returns a list of non-empty line strings."
  (when string
    (remove-if #'(lambda (s) (zerop (length s)))
               (loop for start = 0 then (1+ end)
                     for end = (position #\Newline string :start start)
                     collect (subseq string start end)
                     while end))))

(defun split-string-by-whitespace (string)
  "Split a string by any whitespace (space, tab, newline).
   Returns a list of non-empty strings."
  (when string
    (let ((result '())
          (start 0))
      (loop for i from 0 below (length string)
            do (when (member (char string i) '(#\Space #\Tab #\Newline))
                 (when (> i start)
                   (push (subseq string start i) result))
                 (setf start (1+ i)))
            finally (when (< start (length string))
                      (push (subseq string start) result)))
      (nreverse result))))

(defun strip-type-decorators (object-name)
  "Remove BRL-CAD type decorators from object names.
   Removes trailing '/', 'R', and '/R' suffixes.
   Returns: clean object name."
  (when object-name
    (let ((len (length object-name)))
      (cond
        ;; Remove '/R' suffix
        ((and (>= len 2) (string= (subseq object-name (- len 2)) "/R"))
         (subseq object-name 0 (- len 2)))
        ;; Remove '/' suffix  
        ((and (>= len 1) (char= (char object-name (- len 1)) #\/))
         (subseq object-name 0 (- len 1)))
        ;; No decorators, return as-is
        (t object-name)))))

(defun parse-ls-line (line)
  "Parse a long-format ls line into a plist.
   Long format: name type major-type minor-type length
   Returns: (:name \"name\" :type \"type\" :major-type \"n\" :minor-type \"n\" :length \"n\")"
  (let ((fields (split-string-by-whitespace line)))
    (when (>= (length fields) 5)
      (list :name (nth 0 fields)
            :type (nth 1 fields)
            :major-type (nth 2 fields)
            :minor-type (nth 3 fields)
            :length (nth 4 fields)))))

(defun parse-ls-output (string long-format)
  "Parse ls command output into a list.
   For regular format: returns list of object name strings.
   For long format: returns list of plists with object information."
  (when string
    (if long-format
        ;; Long format: parse each line into structured data
        (let ((lines (split-lines string)))
          (remove nil (mapcar #'parse-ls-line lines)))
        ;; Regular format: split by whitespace to get names
        (mapcar #'strip-type-decorators (split-string-by-whitespace string)))))

(defun validate-alist-attributes (attributes &optional context)
  "Validate that ATTRIBUTES is a proper alist of (string . string) pairs.
   
   Arguments:
     ATTRIBUTES - The attributes list to validate
     CONTEXT - Optional string describing the context for error messages
   
   Returns:
     T if valid
   
   Signals:
     ERROR if validation fails with descriptive message"
  
  (when attributes
    ;; Check that it's a list
    (unless (listp attributes)
      (error "~AATTRIBUTES must be a list, got: ~A" 
             (if context (format nil "~A: " context) "")
             (type-of attributes)))
    
    ;; Check each element is a proper cons pair with strings
    (loop for item in attributes
          for position from 1
          do (unless (consp item)
               (error "~AAttribute at position ~D must be a cons pair, got: ~A"
                      (if context (format nil "~A: " context) "")
                      position
                      (type-of item)))
             (unless (stringp (car item))
               (error "~AAttribute name must be a string, got: ~A at position ~D"
                      (if context (format nil "~A: " context) "")
                      (type-of (car item))
                      position))
             (unless (stringp (cdr item))
               (error "~AAttribute value must be a string, got: ~A at position ~D"
                      (if context (format nil "~A: " context) "")
                      (type-of (cdr item))
                      position))))
  t)

(defun build-flag-args (flags)
  "Build argument list from flag specifications.
   FLAGS is a plist like (:color #(255 0 0) :shaded t :no-resize t)
   Returns list of strings like (\"-C\" \"255/0/0\" \"-m1\" \"-R\")"
  (let ((args '()))
    (loop for (key value) on flags by #'cddr
          do (case key
               (:color 
                (when value
                  (push "-C" args)
                  (push (color->string value) args)))
               (:shaded 
                (when value (push "-m1" args)))
               (:no-resize 
                (when value (push "-R" args)))
               (:simplified 
                (when value (push "-S" args)))
               (:wireframe-threshold
                (when value
                  (push "-L" args)
                  (push (princ-to-string value) args)))
               (:shape-lines
                (when value (push "-s" args)))
               (:all 
                (when value (push "-a" args)))
               (:primitives 
                (when value (push "-p" args)))
               (:regions 
                (when value (push "-r" args)))
               (:combinations 
                (when value (push "-c" args)))
               (:long-format 
                (when value (push "-l" args)))
               (:human-readable 
                (when value (push "-H" args)))
               (:sort-by-size 
                (when value (push "-S" args)))
               (:quiet 
                (when value (push "-q" args)))
               (:match-any
                (when value (push "-o" args)))
               (:attributes
                (when value (push "-A" args)))))
    (nreverse args)))

;;;; ============================================================================
;;;; Database Management Functions
;;;; ============================================================================

(defun database-exists-p (filename)
  "Check if a database file exists and is readable.
   
   Arguments:
     FILENAME - String path to the database file
   
   Returns:
     T if file exists and is readable, NIL otherwise"
  (and (stringp filename)
       (plusp (length filename))
       (probe-file filename)
       (file-readable-p filename)))

(defun get-current-database ()
  "Get the name of the currently open database.
   
   Returns:
     String containing the current database filename, or NIL if no database is open"
  (let ((result (mged:opendb)))
    (when (and result (plusp (length result)))
      result)))

(defun create-database (filename &key overwrite)
  "Create a new database file.
   
   Arguments:
     FILENAME - String path for the new database file
   
   Keyword arguments:
     :OVERWRITE - If T, overwrite existing database file (default: NIL)
   
   Returns:
     T if database was created successfully, NIL on error
   
   Notes:
     - If file already exists and :OVERWRITE is NIL, returns NIL
     - Uses opendb with -c flag to force creation
     - Automatically closes any existing database first"
  
  ;; Validate filename
  (unless (and filename (plusp (length filename)))
    (error "FILENAME must be a non-empty string"))
  
  ;; Check if file exists and overwrite is not specified
  (when (and (database-exists-p filename) (not overwrite))
    (return-from create-database nil))
  
  ;; Close current database if any
  (when (get-current-database)
    (close-database))
  
  ;; Create the new database
  (handler-case
      (progn
        (apply #'mged:opendb `("-c" ,filename))
        (when (get-current-database)
          (format t "Database '~A' created successfully~%" filename))
        t)
    (mged-error ()
      nil)))

(defun open-database (filename &key create-mode)
  "Open an existing database file.
   
   Arguments:
     FILENAME - String path to the database file
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create database if it doesn't exist
     :FORCE-CREATE - If T, force creation (equivalent to -c flag)
     :NO-CREATE - If T, don't create database if it doesn't exist
   
   Returns:
     T if database was opened successfully, NIL on error
   
   Notes:
     - Only one of :CREATE-IF-MISSING, :FORCE-CREATE, :NO-CREATE should be specified
     - :FORCE-CREATE takes precedence over other options
     - :NO-CREATE takes precedence over :CREATE-IF-MISSING
     - Automatically closes any existing database first"
  
  ;; Validate filename
  (unless (and filename (plusp (length filename)))
    (error "FILENAME must be a non-empty string"))
  
  ;; Validate create-mode parameter
  (unless (member create-mode '(:create-if-missing :force-create :no-create :default nil))
    (error "CREATE-MODE must be one of: :CREATE-IF-MISSING, :FORCE-CREATE, :NO-CREATE, :DEFAULT, or NIL"))
  
  ;; Set default mode if NIL
  (let ((actual-create-mode (or create-mode :default)))
    ;; Check if file exists
    (let ((file-exists (database-exists-p filename)))
      (cond
        ;; File doesn't exist - check creation options
        ((not file-exists)
         (case actual-create-mode
           (:force-create
            ;; Force create
            (create-database filename))
           (:no-create
            ;; Don't create
            (format t "Database '~A' does not exist and creation is disabled~%" filename)
            nil)
           ((:create-if-missing :default)
            ;; Create if missing (default behavior)
            (create-database filename))
           (t
            ;; Should not reach here due to validation above
            (error "Invalid create-mode: ~A" actual-create-mode))))
        
        ;; File exists - open it
        (t
         (close-database)
         (handler-case
             (progn
               (mged:opendb filename)
               (when (get-current-database)
                 (format t "Database '~A' opened successfully~%" filename))
               t)
           (mged-error ()
             nil)))))))

(defun close-database ()
  "Close the currently open database.
   
   Returns:
     T if database was closed successfully, NIL if no database was open or on error"
  
  (when (get-current-database)
    (handler-case
        (progn
          (mged:closedb)
          t)
      (mged-error ()
        nil))))

(defun ensure-database (filename &key create-if-missing force-create)
  "Ensure a database is available, opening existing or creating new as needed.
   
   Arguments:
     FILENAME - String path to the database file
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create database if it doesn't exist (default: T)
     :FORCE-CREATE - If T, force creation of new database
   
   Returns:
     T if database is available (opened or created), NIL on error
   
   Notes:
     - This is a convenience function that handles both existing and new databases
     - Default behavior is to create if missing
     - :FORCE-CREATE creates a new database even if one exists"
  
  (let ((file-exists (database-exists-p filename)))
    (cond
      (force-create
       ;; Force create new database
       (create-database filename))
      (file-exists
       ;; Open existing database
       (open-database filename))
      (create-if-missing
       ;; Create if missing (default behavior)
       (create-database filename))
      (t
       ;; Don't create, just try to open
       (open-database filename :no-create t)))))

(defmacro with-database (filename create-mode &rest body)
  "Execute BODY with the specified database temporarily open.
   
   Arguments:
     FILENAME - String path to the database file
     CREATE-MODE - Symbol specifying creation behavior:
                    :CREATE-IF-MISSING - Create if file doesn't exist
                    :FORCE-CREATE - Force creation of new file
                    :NO-CREATE - Don't create, only open existing
                    :DEFAULT - Use default behavior (create if missing)
   
   Body:
     Forms to execute with the database open
   
   Returns:
     Value of the last form in BODY
   
   Notes:
     - Saves current database state before opening new one
     - Restores original database state after BODY completes
     - Uses unwind-protect to ensure cleanup even if BODY errors
     - If no database was open originally, closes database after BODY"
  
  (let ((original-db (gensym "ORIGINAL-DB"))
        (success-var (gensym "SUCCESS"))
        (result-var (gensym "RESULT")))
    `(let ((,original-db (get-current-database))
           (,success-var nil)
           (,result-var nil))
       (unwind-protect
           (progn
             ;; Open the specified database with the new create-mode parameter
             (when (open-database ,filename 
                                  :create-mode ,create-mode)
               (setf ,success-var t)
               ;; Execute the body
               (setf ,result-var (progn ,@body))))
         
         ;; Cleanup: restore original database state
         (when ,success-var
           (let ((current-db (get-current-database)))
             ;; Close current database
             (when current-db
               (close-database))
             
             ;; Restore original database if there was one
             (when ,original-db
               (open-database ,original-db)))))
       
       ;; Return the result
       ,result-var)))

;;;; ============================================================================
;;;; Listing & Query Functions
;;;; ============================================================================

(defun list-objects (&key all primitives regions combinations 
                          pattern attributes match-any long-format 
                          human-readable sort-by-size quiet)
  "List objects in the database with various filters.
  
  Keyword arguments:
    :ALL - List all objects (including hidden)
    :PRIMITIVES - List only primitives
    :REGIONS - List only regions
    :COMBINATIONS - List only combinations
    :PATTERN - String pattern to match (supports wildcards)
    :ATTRIBUTES - Alist of attribute name/value pairs to match
    :MATCH-ANY - If T with :ATTRIBUTES, match any attribute (OR logic)
    :LONG-FORMAT - If T, return detailed information as plists
    :HUMAN-READABLE - If T with :LONG-FORMAT, use human-readable sizes
    :SORT-BY-SIZE - If T, sort by object size
    :QUIET - If T, suppress informational messages
  
  Returns:
    - If :LONG-FORMAT is NIL: List of object name strings
    - If :LONG-FORMAT is T: List of plists with keys:
        :NAME - Object name
        :TYPE - Object type
        :MAJOR-TYPE - Major type code
        :MINOR-TYPE - Minor type code
        :LENGTH - Object size/length
    Returns NIL on error"
  
  (let ((args (build-flag-args 
               (list :all all
                     :primitives primitives
                     :regions regions
                     :combinations combinations
                     :long-format long-format
                     :human-readable human-readable
                     :sort-by-size sort-by-size
                     :quiet quiet
                     :attributes attributes
                     :match-any match-any))))
    
    ;; Add attribute pairs if provided
    (when attributes
      (dolist (attr-pair attributes)
        (push (car attr-pair) args)
        (push (cdr attr-pair) args)))
    
    ;; Add pattern if provided
    (when pattern
      (push pattern args))
    
    ;; Get result from mged:ls and parse into appropriate format
    (let ((result (apply #'mged:ls (nreverse args))))
      (parse-ls-output result long-format))))

(defun object-exists-p (name)
  "Check if an object exists in the database.
  
  Arguments:
    NAME - String name of the object to check
  
  Returns:
    T if object exists, NIL otherwise"
  (handler-case
      (progn
        (mged:exists name)
        t)
    (mged-error () nil)))

(defun get-object-info (name &key attributes)
  "Get information about an object.
  
  Arguments:
    NAME - String name of the object
    :ATTRIBUTES - If T, also return attributes as structured data
  
  Returns:
    - If ATTRIBUTES is NIL: String containing object information
    - If ATTRIBUTES is T: plist of attribute-name -> value pairs
    - NIL on error"
  (if attributes
      (get-attributes name)
      (mged:get name)))

;;;; ============================================================================
;;;; Display Operations
;;;; ============================================================================

(defun draw-objects (names &key color shaded wireframe-threshold 
                           no-resize simplified shape-lines
                           attributes match-any)
  "Draw objects on the display.
  
  Arguments:
    NAMES - String or list of strings specifying object names
  
  Keyword arguments:
    :COLOR - Vector #(r g b) or list (r g b) specifying override color
    :SHADED - If T, draw shaded (requires OpenGL + zbuffer + lighting)
    :WIREFRAME-THRESHOLD - Integer face count for BOT wireframe threshold
    :NO-RESIZE - If T, don't auto-resize view
    :SIMPLIFIED - If T, skip subtractions and simplify display
    :SHAPE-LINES - If T, draw subtracted/intersected with shape lines
    :ATTRIBUTES - Alist of attribute name/value pairs to filter objects
    :MATCH-ANY - If T with :ATTRIBUTES, match any attribute (OR logic)
  
  Returns:
    Result string from draw command, or NIL on error"
  
  (let* ((name-list (ensure-list names))
         (args (build-flag-args 
                (list :color color
                      :shaded shaded
                      :wireframe-threshold wireframe-threshold
                      :no-resize no-resize
                      :simplified simplified
                      :shape-lines shape-lines
                      :attributes attributes
                      :match-any match-any))))
    
    ;; Add attribute pairs if provided
    (when attributes
      (dolist (attr-pair attributes)
        (push (car attr-pair) args)
        (push (cdr attr-pair) args)))
    
    (apply #'mged:draw (append (nreverse args) name-list))))

(defun erase-objects (names)
  "Erase objects from the display.
  
  Arguments:
    NAMES - String or list of strings specifying object names to erase
  
  Returns:
    Result string from erase command"
  (let ((name-list (ensure-list names)))
    (apply #'mged:erase name-list)))

(defun clear-display ()
  "Clear all objects from the display (equivalent to 'Z' command).
  
  Returns:
    Result from the Z command"
  (mged:Z))

;;;; ============================================================================
;;;; Primitive Creation Functions
;;;; ============================================================================

(defun create-primitive (name type &rest parameters)
  "Low-level primitive creation (wrapper around 'in' command).
   Automatically expands vectors into separate arguments.
  
  Arguments:
    NAME - String name for the new primitive
    TYPE - String primitive type (e.g., 'sph', 'rcc', 'arb8')
    PARAMETERS - Remaining arguments (vectors automatically expanded)
  
  Returns:
    Result string from 'in' command"
  
  (let ((expanded-params 
         (mapcan (lambda (param)
                   (etypecase param
                     (vector 
                      ;; Expand vector into separate string arguments
                      (map 'list #'princ-to-string (coerce param 'list)))
                     (number 
                      (list (princ-to-string param)))
                     (string 
                      (list param))))
                 parameters)))
    (apply #'mged:in name type expanded-params)))

(defun make-sphere (name center radius)
  "Create a sphere primitive.
  
  Arguments:
    NAME - String name for the sphere
    CENTER - Vector #(x y z) specifying sphere center
    RADIUS - Number specifying sphere radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "sph" center radius))

(defun make-ellipsoid (name center a b c)
  "Create an ellipsoid primitive.
  
  Arguments:
    NAME - String name for the ellipsoid
    CENTER - Vector #(x y z) specifying ellipsoid center
    A - Vector #(x y z) specifying first semi-axis
    B - Vector #(x y z) specifying second semi-axis
    C - Vector #(x y z) specifying third semi-axis
  
  Returns:
    Result string from creation command"
  (create-primitive name "ell" center a b c))

(defun make-cylinder (name base height radius)
  "Create a right circular cylinder (RCC).
  
  Arguments:
    NAME - String name for the cylinder
    BASE - Vector #(x y z) specifying base center point
    HEIGHT - Vector #(x y z) specifying height vector
    RADIUS - Number specifying cylinder radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "rcc" base height radius))

(defun make-cone (name base height base-radius top-radius)
  "Create a truncated right circular cone (TRC).
  
  Arguments:
    NAME - String name for the cone
    BASE - Vector #(x y z) specifying base center point
    HEIGHT - Vector #(x y z) specifying height vector
    BASE-RADIUS - Number specifying base radius
    TOP-RADIUS - Number specifying top radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "trc" base height base-radius top-radius))

(defun make-box (name corner dimensions)
  "Create a box (ARB6 with specific vertex arrangement).
  
  Arguments:
    NAME - String name for the box
    CORNER - Vector #(x y z) specifying one corner
    DIMENSIONS - Vector #(w h d) specifying width, height, depth
  
  Returns:
    Result string from creation command"
  (let ((x (aref corner 0))
        (y (aref corner 1))
        (z (aref corner 2))
        (w (aref dimensions 0))
        (h (aref dimensions 1))
        (d (aref dimensions 2)))
    ;; Create an RPP (right rectangular parallelepiped)
    (create-primitive name "rpp" 
                      (vector x y z)
                      (vector (+ x w) (+ y h) (+ z d)))))

(defun make-arb8 (name vertices)
  "Create an ARB8 (arbitrary 8-vertex polyhedron).
  
  Arguments:
    NAME - String name for the ARB8
    VERTICES - List of 8 vectors, each #(x y z), specifying vertices
  
  Returns:
    Result string from creation command"
  (unless (= (length vertices) 8)
    (error "ARB8 requires exactly 8 vertices, got ~D" (length vertices)))
  (apply #'create-primitive name "arb8" vertices))

(defun make-torus (name center normal inner-radius outer-radius)
  "Create a torus primitive.
  
  Arguments:
    NAME - String name for the torus
    CENTER - Vector #(x y z) specifying torus center
    NORMAL - Vector #(x y z) specifying normal direction
    INNER-RADIUS - Number specifying tube radius
    OUTER-RADIUS - Number specifying distance from center to tube center
  
  Returns:
    Result string from creation command"
  (create-primitive name "tor" center normal inner-radius outer-radius))

(defun make-particle (name base height base-radius top-radius)
  "Create a particle primitive.
  
  Arguments:
    NAME - String name for the particle
    BASE - Vector #(x y z) specifying base point
    HEIGHT - Vector #(x y z) specifying height vector
    BASE-RADIUS - Number specifying base radius
    TOP-RADIUS - Number specifying top radius (0 for point)
  
  Returns:
    Result string from creation command"
  (create-primitive name "part" base height base-radius top-radius))

;;;; ============================================================================
;;;; Boolean Expression Utilities
;;;; ============================================================================

(defun interleave-operator (operands operator)
  "Interleave OPERATOR between each pair of OPERANDS.
   OPERANDS is a flat list of processed operands.
   OPERATOR is the operator string to insert."
  (case (length operands)
    (0 '())
    (1 operands)
    (t (cons (first operands)
             (cons operator
                   (interleave-operator (rest operands) operator))))))

(defun expression->boolean-list (expression)
  "Convert Lisp boolean expression to BRL-CAD format list.
   Operators: symbols (union, subtract, intersect)
   Operands: strings (object names)
   
   The BRL-CAD c command expects a list where each space in the expression
   becomes a separate list element.
   
   Examples:
     (expression->boolean-list '(union \"sphere1\" \"sphere2\"))
     => '(\"(\" \"sphere1\" \"u\" \"sphere2\" \")\")
     
     (expression->boolean-list '(union \"sphere1\" (subtract \"sphere2\" (intersect \"sphere3\" \"sphere4\"))))
     => '(\"(\" \"sphere1\" \"u\" \"(\" \"sphere2\" \"-\" \"(\" \"sphere3\" \"+\" \"sphere4\" \")\" \")\" \")\")
   
   This list can then be passed to mged:c as: (apply #'mged:c \"-c\" name boolean-list)"
  (etypecase expression
    (string 
     ;; Leaf node - return as single-element list
     (list expression))
    (list 
     ;; List with symbol operator and string operands
     (let* ((op (first expression))
           (operands (mapcan #'expression->boolean-list (rest expression)))
           (op-str (case op (union "u") (subtract "-") (intersect "+"))))
       (cons "(" 
             (append (interleave-operator operands op-str)
                     (list ")")))))))

(defun validate-boolean-expression (expression)
  "Validate expression: symbol operators, string operands only
   
   Validates that:
   - Operators are symbols: union, subtract, intersect
   - Operands are non-empty strings
   - Each operation has at least 2 operands"
  (labels ((validate (expr)
             (etypecase expr
               (string 
                (unless (plusp (length expr))
                  (error "Object names must be non-empty strings, got: \"~A\"" expr)))
               (list
                (unless (symbolp (first expr))
                  (error "Boolean operators must be symbols, got: ~A" (type-of (first expr))))
                (unless (member (first expr) '(union subtract intersect))
                  (error "Unknown boolean operation: ~A. Use: union, subtract, or intersect" (first expr)))
                (when (< (length expr) 3)
                  (error "Operation ~A requires at least 2 operands, got: ~D" 
                         (first expr) (1- (length expr))))
                (mapc #'validate (rest expr))))))
    (validate expression)))

;;;; ============================================================================
;;;; Combinations & Regions
;;;; ============================================================================

(defun make-combination (name expression &key color shader)
  "Create a combination using intuitive boolean expression syntax.
   
   EXPRESSION: Lisp list with symbol operators and string operands
   Operators: union, subtract, intersect
   Operands: Strings containing object names
   
   Examples:
     (make-combination 'simple' '(union \"sphere1\" \"sphere2\"))
     (make-combination 'complex' '(union \"sphere1\" (subtract \"sphere2\" (intersect \"sphere3\" \"sphere4\"))))
     (make-combination 'nested' '(union \"base\" (subtract \"part1\" (intersect \"hole1\" \"hole2\")) \"part2\"))
   
   Keyword arguments:
     :COLOR - Vector #(r g b) or list (r g b) specifying combination color
     :SHADER - String specifying shader name
   
   Returns:
     Result string from 'c' command"
  
  (validate-boolean-expression expression)
  (let* ((expr-list (expression->boolean-list expression))
         (combination-return (apply #'mged:c "-c" name expr-list)))
         (when color (mged:mater name "" "" (color->string color) ""))
    (when shader (mged:mater name shader "" "" ""))
    combination-return))

(defun make-region (name expression &key id color shader material attributes)
  "Create a region using intuitive boolean expression syntax.
   
   Same expression format as make-combination, but creates a region with
   region-specific attributes.
   
   Examples:
     (make-region 'steel_part' '(union \"base\" (subtract \"raw\" \"cutter\")) 
                   :id 100 :material \"steel\")
     (make-region 'complex_region' 
                   '(union \"sphere1\" (subtract \"block\" (intersect \"cylinder1\" \"cylinder2\")))
                   :id 101 :color #(255 0 0) :attributes '((\"surface\" . \"polished\")))
   
   Keyword arguments:
     :ID - Integer region ID
     :COLOR - Vector #(r g b) or list (r g b) specifying region color
     :SHADER - String specifying shader name
     :MATERIAL - String specifying material name
     :ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                   '((\"material_id\" . \"10\") (\"custom_prop\" . \"value\"))
   
   Returns:
     Result string from 'c' command"
  
  (validate-boolean-expression expression)
  (let* ((expr-list (expression->boolean-list expression))
        (combination-return (apply #'mged:c "-r" name expr-list)))
        ;; Set region-specific attributes
        (when id (set-attributes name `(("region_id" . ,(princ-to-string id)))))
        (when color (mged:mater name "" "" (color->string color) ""))
        (when shader (mged:mater name shader "" "" ""))
        (when material (mged:mater name "" material "" ""))
    (when attributes 
      (validate-alist-attributes attributes "make-region")
      (apply #'mged:attr "set" name 
             (mapcan (lambda (pair) (list (car pair) (cdr pair))) attributes)))
    combination-return))

(defun make-group (name members &key color shader)
  "Create a group (union combination) from a list of object names.
   
   This is a convenience function for creating simple union combinations.
   Equivalent to: (make-combination name (cons 'union members))
   
   MEMBERS: List of strings containing object names
   
   Examples:
     (make-group 'assembly' '(\"sphere1\" \"sphere2\" \"cube1\"))
     (make-group 'parts' (list-objects :pattern \"part*\") :color #(0 255 0))
   
   Keyword arguments:
     :COLOR - Vector #(r g b) or list (r g b) specifying group color
     :SHADER - String specifying shader name
   
   Returns:
     Result from make-combination"
  
  (unless (and members (every #'stringp members))
    (error "MEMBERS must be a non-empty list of strings, got: ~A" members))
  
  ;; Convert to union expression and delegate to make-combination
  (make-combination name (cons 'union members)
                    :color color :shader shader))

(defun make-lathe (name shape-function start-point length-vector step-length)
  "Create a lathe object by revolving a shape function along an axis.
   
   This function creates a lathe shape by generating a series of cone sections
   that approximate the profile defined by SHAPE-FUNCTION. The shape function
   takes an offset distance and returns the radius at that offset.
   
   Arguments:
     NAME - String name for the lathe group
     SHAPE-FUNCTION - Function that takes an offset (number) and returns a radius
     START-POINT - Vector #(x y z) specifying the starting point of the lathe axis
     LENGTH-VECTOR - Vector #(dx dy dz) specifying the direction and length of the lathe
     STEP-LENGTH - Number specifying the distance between cone sections
   
   Returns:
     Result from make-group command containing all cone sections
   
   Example:
     ;; Create a simple cylinder lathe
     (make-lathe 'cylinder
                 (lambda (offset) 5.0)  ; Constant radius of 5
                 #(0 0 0)               ; Start at origin
                 #(0 0 10)              ; 10 units along Z axis
                 1.0)                   ; 1 unit steps
   
   Notes:
     - Creates multiple cone primitives and combines them into a group
     - The shape function should return non-negative radius values
     - Smaller step-length creates smoother but more complex geometry"
  
  (let ((direction (mged-math::normalize length-vector))
        (overall-length (mged-math::magnitude length-vector))
        (created-section-names '())) 
    (flet ((increment-offset (current-offset) 
             (let ((expected-next-offset (+ current-offset step-length)))
               (if (>= expected-next-offset overall-length) 
                   overall-length 
                   expected-next-offset)))) 
      (do* ((section-count 0 (+ section-count 1))
             (current-offset 0 (setq current-offset next-offset))
             (next-offset  (increment-offset current-offset) (increment-offset current-offset))
             (section-length (- next-offset current-offset)))
        ((>= next-offset overall-length) (mged-api::make-group name created-section-names))
        (let* ( 
               (start-radius (abs (funcall shape-function current-offset)))
               (end-radius (abs (funcall shape-function next-offset)))
               (section-length-vector (mged-math::v* direction section-length))
               (current-start-point (mged-math::v+ start-point 
                                                  (mged-math::v* direction current-offset)))
               (section-name (format nil "section-~A-~A" section-count name)))
          (setq created-section-names 
                (cons (mged-api::make-cone section-name
                                           current-start-point 
                                           section-length-vector 
                                           start-radius 
                                           end-radius) 
                      created-section-names)))))))

;;;; ============================================================================
;;;; Object Manipulation
;;;; ============================================================================

(defun copy-object (old-name new-name)
  "Copy an object to a new name.
  
  Arguments:
    OLD-NAME - String name of existing object
    NEW-NAME - String name for the copy
  
  Returns:
    Result string from 'cp' command"
  (mged:cp old-name new-name))

(defun move-object (old-name new-name)
  "Rename/move an object.
  
  Arguments:
    OLD-NAME - String name of existing object
    NEW-NAME - String new name for the object
  
  Returns:
    Result string from 'mv' command"
  (mged:mv old-name new-name))

;;;; ============================================================================
;;;; Object Deletion Operations
;;;; ============================================================================

(defun parse-kill-output (output)
  "Parse kill command output to extract object names.
   Used for dry-run mode (-n flag) to return structured data.
   
   Arguments:
     OUTPUT - String output from kill/killall/killtree with -n flag
   
   Returns:
     List of object name strings that would be killed"
  (when output
    (let ((lines (split-lines output)))
      (remove-if #'(lambda (s) (zerop (length s)))
                 (mapcar #'(lambda (line)
                           (string-trim "(\"" (string-trim ")\"" line)))
                         lines)))))

(defun build-kill-flags (flags)
  "Build argument list for kill command flags.
   FLAGS is a plist like (:force t :quiet t)
   Returns list of flag strings."
  (let ((args '()))
    (loop for (key value) on flags by #'cddr
          do (case key
               (:force 
                (when value (push "-f" args)))
               (:quiet 
                (when value (push "-q" args)))
               (:dry-run 
                (when value (push "-n" args)))
               (:all 
                (when value (push "-a" args)))))
    (nreverse args)))

(defun kill-objects (&optional names &key force quiet)
  "Delete specified objects from the database.
   
   This is a high-level wrapper for the 'kill' command with Lisp-idiomatic
   keyword arguments. Objects are deleted immediately - there is no undo.
   
   Arguments:
     NAMES - Optional string or list of strings specifying object names to delete.
             If NIL or omitted, deletes ALL objects in the database.
   
   Keyword arguments:
     :FORCE - If T, don't complain if some objects don't exist (maps to -f flag)
     :QUIET - If T, suppress database object lookup failure messages (maps to -q flag)
   
   Returns:
     Result string from kill command, or NIL on error
   
   Example:
     ;; Delete all objects in database
     (kill-objects)
     
     ;; Delete specific objects
     (kill-objects '(\"sphere1\" \"box2\"))
     
     ;; Delete with force flag (no complaints about missing objects)
     (kill-objects \"temp_obj\" :force t)
     
     ;; Delete quietly
     (kill-objects '(\"obj1\" \"obj2\") :quiet t)
   
   Warning: This operation is destructive and cannot be undone. Use with caution."
  
  (let* ((target-names (if names
                           (ensure-list names)
                           ;; If no names specified, get all objects
                           (list-objects)))
         (flag-args (build-kill-flags (list :force force :quiet quiet))))
    (apply #'mged:kill (append flag-args target-names))))

(defun kill-objects-and-references (names &key dry-run)
  "Delete specified objects and remove all references to them from combinations.
   
   This is a high-level wrapper for the 'killall' command with enhanced
   functionality. It removes objects and cleans up all references to them
   from combinations in the database.
   
   Arguments:
     NAMES - Required string or list of strings specifying object names to delete.
   
   Keyword arguments:
     :DRY-RUN - If T, return list of objects that would be killed without
                actually deleting them (maps to -n flag)
   
   Returns:
     - Normal mode: Result string from killall command, or NIL on error
     - Dry-run mode: List of object name strings that would be killed
   
   Example:
     ;; Delete specific objects and all references
     (kill-objects-and-references '(\"sphere1\" \"box2\"))
     
     ;; Dry run to see what would be deleted
     (kill-objects-and-references \"sphere1\" :dry-run t)
     
     ;; Dry run for specific objects
     (kill-objects-and-references '(\"temp1\" \"temp2\") :dry-run t)
   
   Warning: This operation is destructive and cannot be undone. 
            Consider using :DRY-RUN T first to verify what will be deleted."
  
  (let* ((name-list (ensure-list names))
         (flag-args (build-kill-flags (list :dry-run dry-run))))
    
    (if dry-run
        ;; Dry run mode - parse and return object list
        (let ((result (apply #'mged:killall (append flag-args name-list))))
          (parse-kill-output result))
        ;; Normal mode - execute deletion
        (apply #'mged:killall (append flag-args name-list)))))

(defun kill-object-tree (names &key all force dry-run)
  "Delete specified objects and recursively delete all objects they reference.
   
   This is a high-level wrapper for the 'killtree' command with Lisp-idiomatic
   keyword arguments. For each combination among the specified objects, the
   combination and all its members are deleted recursively.
   
   Arguments:
     NAMES - String or list of strings specifying object names to delete
   
   Keyword arguments:
     :ALL - If T, kill objects even if referenced elsewhere, then kill all
            references (maps to -a flag, equivalent to killall on each member)
     :FORCE - If T, kill objects even if referenced elsewhere (may create
              dangling references, maps to -f flag)
     :DRY-RUN - If T, return list of objects that would be killed without
                actually deleting them (maps to -n flag)
   
   Returns:
     - Normal mode: Result string from killtree command, or NIL on error
     - Dry-run mode: List of object name strings that would be killed
   
   Example:
     ;; Delete object tree recursively
     (kill-object-tree \"assembly1\")
     
     ;; Delete with all references (safer than :force)
     (kill-object-tree '(\"group1\" \"group2\") :all t)
     
     ;; Force delete (may create dangling references)
     (kill-object-tree \"complex_assembly\" :force t)
     
     ;; Dry run to see what would be deleted
     (kill-object-tree \"assembly1\" :dry-run t)
   
   Note: :ALL flag is generally safer than :FORCE as it cleans up references.
   
   Warning: This operation is destructive and cannot be undone. 
            Consider using :DRY-RUN T first to verify what will be deleted."
  
  (let* ((name-list (ensure-list names))
         (flag-args (build-kill-flags (list :all all :force force :dry-run dry-run))))
    
    (if dry-run
        ;; Dry run mode - parse and return object list
        (let ((result (apply #'mged:killtree (append flag-args name-list))))
          (parse-kill-output result))
        ;; Normal mode - execute deletion
        (apply #'mged:killtree (append flag-args name-list)))))

;;;; ============================================================================
;;;; Transformation Functions
;;;; ============================================================================

(defun translate-object (offset)
  "Translate the currently edited object by an offset vector.
  
  Arguments:
    OFFSET - Vector #(dx dy dz) specifying translation offset
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'tra' command"
  (mged:tra (aref offset 0) (aref offset 1) (aref offset 2)))

(defun rotate-object (angles &key about-point)
  "Rotate the currently edited object.
  
  Arguments:
    ANGLES - Vector #(rx ry rz) specifying rotation angles in degrees
  
  Keyword arguments:
    :ABOUT-POINT - Vector #(x y z) specifying point to rotate about (optional)
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'rot' command"
  (if about-point
      ;; Rotate about a specific point (using center command first)
      (progn
        (mged:center (vector->string about-point))
        (mged:rot (aref angles 0) (aref angles 1) (aref angles 2)))
      ;; Rotate about current center
      (mged:rot (aref angles 0) (aref angles 1) (aref angles 2))))

(defun scale-object (factor &key uniform)
  "Scale the currently edited object.
  
  Arguments:
    FACTOR - Number or vector #(sx sy sz) specifying scale factor(s)
  
  Keyword arguments:
    :UNIFORM - If T, FACTOR must be a number and applies uniformly
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'sca' command"
  (if uniform
      (mged:sca (if (numberp factor) factor (error "Uniform scale requires a number")))
      (if (vectorp factor)
          (mged:sca (aref factor 0) (aref factor 1) (aref factor 2))
          (mged:sca factor))))

;;;; ============================================================================
;;;; Attribute Management Utility Functions
;;;; ============================================================================

(defun parse-attribute-output (output)
  "Parse attribute command output into structured data.
   
   Arguments:
     OUTPUT - String output from attr command
     SINGLE-OBJECT-P - If T, parse for single object (returns plist)
                      If NIL, parse for multiple objects (returns list of plists)
   
   Returns:
     List of (object-name . attribute-plist) pairs"
  (when output
    (let ((lines (split-lines output)))
      (remove-if #'null
                 (mapcar (lambda (line)
                           (when (and line (plusp (length line)))
                             (let ((trimmed (string-trim " " line)))
                               ;; Parse multiple objects: object_name attr_name value
                               (let* ((parts (split-string-by-whitespace trimmed))
                                      (object-name (first parts))
                                      (attr-name (second parts))
                                      (attr-value (third parts)))
                                 (when (and object-name attr-name attr-value)
                                   (cons object-name
                                         (list (intern (string-upcase attr-name) :keyword)
                                               attr-value)))))))
                         lines)))))

(defun normalize-attribute-pairs (attributes)
  "Normalize attribute specifications to a list of (name . value) pairs.
   
   Arguments:
     ATTRIBUTES - Can be:
                  - Alist of (name . value) pairs
                  - Plist of name value name value...
                  - List of two-element lists ((name value) ...)
   
   Returns:
     List of (name . value) cons pairs"
  (when attributes
    (etypecase attributes
      (list
       (cond
         ;; Check if it's an alist (first element is a cons)
         ((and (first attributes) (consp (first attributes)))
          (mapcar (lambda (pair)
                    (if (consp pair)
                        pair
                        (error "Invalid attribute pair: ~A" pair)))
                  attributes))
         ;; Check if it's a plist (even number of elements, first is not a cons)
         ((evenp (length attributes))
          (loop for (name value) on attributes by #'cddr
                collect (cons name value)))
         (t
          (error "Attribute list must have even number of elements or be an alist")))
       (t
        (error "Attributes must be a list"))))))

(defun build-attribute-args (attribute-pairs)
  "Convert attribute pairs to flat argument list for attr command.
   
   Arguments:
     ATTRIBUTE-PAIRS - List of (name . value) cons pairs
   
   Returns:
     Flat list of name value name value..."
  (when attribute-pairs
    (mapcan (lambda (pair)
              (list (car pair) (cdr pair)))
            attribute-pairs)))

;;;; ============================================================================
;;;; Attribute Management Functions
;;;; ============================================================================

(defun get-attributes (object-pattern &key attribute-names all-attributes)
  "Retrieve attributes from objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :ATTRIBUTE-NAMES - List of specific attribute names to retrieve
     :ALL-ATTRIBUTES - If T, retrieve all attributes (default when no specific names)
   
   Returns:
     - Single object match: plist of attribute-name -> value pairs
     - Multiple object matches: list of (object-name . attribute-plist) pairs
     - NIL if no objects match or no attributes found
   
   Examples:
     ;; Get all attributes for a single object
     (get-attributes \"region1\")
     ;; => (:MATERIAL-ID \"10\" :REGION \"R\" :LOS \"100\")
     
     ;; Get specific attributes
     (get-attributes \"region1\" :attribute-names '(\"material_id\" \"color\"))
     ;; => (:MATERIAL-ID \"10\" :COLOR \"255/0/0\")
     
     ;; Get attributes from multiple objects
     (get-attributes \"region*\")
     ;; => ((\"region1\" (:MATERIAL-ID \"10\" :REGION \"R\"))
     ;;     (\"region2\" (:MATERIAL-ID \"20\" :REGION \"R\")))"
  
  (let* ((attr-names (or attribute-names 
                        (when all-attributes '("*"))
                        '("*")))
         (args (append '("get") (list object-pattern) attr-names))
         (result (apply #'mged:attr args))
         (parsed (parse-attribute-output result)))
    
    ;; Group attributes by object
    (if (null parsed)
        nil
        (let ((object-groups (make-hash-table :test 'equal)))
          ;; Group attributes by object name
          (dolist (item parsed)
            (let* ((object-name (car item))
                   (attr-pair (cdr item))
                   (existing-attrs (gethash object-name object-groups)))
              (setf (gethash object-name object-groups)
                    (append existing-attrs attr-pair))))
          
          ;; Convert to appropriate return format
          (let ((objects (loop for key being the hash-keys of object-groups
                               collect key)))
            (if (= (length objects) 1)
                ;; Single object - return plist
                (gethash (first objects) object-groups)
                ;; Multiple objects - return list of pairs
                (mapcar (lambda (obj-name)
                          (cons obj-name (gethash obj-name object-groups)))
                        objects)))))))

(defun set-attributes (object-pattern attributes)
  "Set attribute values on objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                  '(\"material_id\" . \"10\") (\"color\" . \"255/0/0\"))
                  Only alist format is supported.
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create objects if they don't exist (not implemented)
   
   Returns:
     Result string from attr set command, or NIL on error
   
   Examples:
     ;; Set attributes using alist
     (set-attributes \"region1\" '((\"material_id\" . \"10\") (\"color\" . \"255/0/0\")))
     
     ;; Set attributes on multiple objects
     (set-attributes \"region*\" '((\"region\" . \"R\") (\"los\" . \"100\")))"
  
  ;; Validate attributes format (alist only)
  (validate-alist-attributes attributes "set-attributes")
  
  (let* ((attr-args (build-attribute-args attributes))
         (args (append '("set") (list object-pattern) attr-args)))
    (apply #'mged:attr args)))

(defun remove-attributes (object-pattern attribute-names)
  "Remove specified attributes from objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTE-NAMES - String or list of attribute names to remove
   
   Keyword arguments:
     :QUIET - If T, suppress error messages for non-existent attributes
   
   Returns:
     Result string from attr rm command, or NIL on error
   
   Examples:
     ;; Remove single attribute
     (remove-attributes \"region1\" \"temp_attr\")
     
     ;; Remove multiple attributes
     (remove-attributes \"region*\" '(\"temp_attr\" \"old_attr\") :quiet t)"
  
  (let* ((name-list (ensure-list attribute-names))
         (args (append '("rm") (list object-pattern) name-list)))
    (apply #'mged:attr args)))

(defun append-attributes (object-pattern attributes)
  "Add attributes to objects matching the pattern (append-or-set behavior).
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                  '(\"comment\" . \"Modified part\") (\"version\" . \"2\"))
                  Only alist format is supported.
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create objects if they don't exist (not implemented)
   
   Returns:
     Result string from attr append command, or NIL on error
   
   Examples:
     ;; Append attributes (creates if doesn't exist)
     (append-attributes \"region1\" '((\"comment\" . \"Modified part\") (\"version\" . \"2\")))"
  
  ;; Validate attributes format (alist only)
  (validate-alist-attributes attributes "append-attributes")
  
  (let* ((attr-args (build-attribute-args attributes))
         (args (append '("append") (list object-pattern) attr-args)))
    (apply #'mged:attr args)))

(defun list-attribute-types (object-pattern &key key-filter value-filter)
  "List attribute types present on objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :KEY-FILTER - Pattern to filter attribute names
     :VALUE-FILTER - Pattern to filter attribute values (requires :KEY-FILTER)
   
   Returns:
     - Without filters: List of attribute name strings
     - With key filter only: List of matching attribute name strings  
     - With both filters: List of \"key=value\" strings for matching pairs
   
   Examples:
     ;; List all attribute types in database
     (list-attribute-types \"*\")
     ;; => (\"material_id\" \"region\" \"los\" \"color\" \"shader\")
     
     ;; List attributes matching pattern
     (list-attribute-types \"*\" :key-filter \"material_*\")
     ;; => (\"material_id\" \"material_name\")
     
     ;; List specific attribute values
     (list-attribute-types \"*\" :key-filter \"material_id\" :value-filter \"*\")
     ;; => (\"material_id=1\" \"material_id=2\" \"material_id=10\")"
  
  (let ((args (append '("list") (list object-pattern))))
    (when key-filter
      (push key-filter args)
      (when value-filter
        (push value-filter args)))
    (let ((result (apply #'mged:attr (nreverse args))))
      (when result
        (split-string-by-whitespace result)))))

(defun show-attributes (object-pattern &key attribute-names)
  "Pretty-print attributes for objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :ATTRIBUTE-NAMES - List of specific attribute names to show
   
   Returns:
     Formatted string with pretty-printed attributes, or NIL on error
   
   Examples:
     ;; Show all attributes for an object
     (show-attributes \"region1\")
     
     ;; Show specific attributes
     (show-attributes \"region*\" :attribute-names '(\"material_id\" \"color\"))"
  
  (let ((args (append '("show") (list object-pattern) attribute-names)))
    (apply #'mged:attr args)))

(defun sort-attributes (object-pattern &key sort-type)
  "Display sorted attributes for objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :SORT-TYPE - Sort method: :CASE (default), :NOCASE, :VALUE, :VALUE-NOCASE
   
   Returns:
     Formatted string with sorted attributes, or NIL on error
   
   Examples:
     ;; Sort attributes alphabetically (case-sensitive)
     (sort-attributes \"region1\")
     
     ;; Sort attributes alphabetically (case-insensitive)
     (sort-attributes \"region*\" :sort-type :nocase)
     
     ;; Sort by attribute values
     (sort-attributes \"region1\" :sort-type :value)"
  
  (let* ((sort-str (case sort-type
                    (:case "case")
                    (:nocase "nocase") 
                    (:value "value")
                    (:value-nocase "value-nocase")
                    (t "case")))
         (args (append '("sort") (list object-pattern) (list sort-str))))
    (apply #'mged:attr args)))

(defun copy-attribute (source-object source-attr target-object target-attr)
  "Copy attribute value from source object to target object.
   
   Arguments:
     SOURCE-OBJECT - String name of source object
     SOURCE-ATTR - String name of source attribute
     TARGET-OBJECT - String name of target object  
     TARGET-ATTR - String name of target attribute
   
   Returns:
     Result string from attr copy command, or NIL on error
   
   Examples:
     ;; Copy attribute between objects
     (copy-attribute \"region1\" \"material_id\" \"region2\" \"material_id\")
     
     ;; Copy attribute to new attribute name
     (copy-attribute \"region1\" \"old_id\" \"region1\" \"new_id\")"
  
  (apply #'mged:attr "copy" source-object source-attr target-object target-attr))

;;;; ============================================================================
;;;; Raytracing Functions
;;;; ============================================================================

(defun raytrace (objects &key 
                 ;; Output options
                 (size 512)
                 width
                 height
                 output-file
                 framebuffer
                 
                 ;; View options
                 background-color
                 perspective
                 azimuth
                 elevation
                 aspect-ratio
                 
                 ;; Quality options
                 (lighting-model 0)
                 ambient-light
                 hypersample
                 jitter
                 processors
                 
                 ;; Advanced options
                 benchmark
                 report-overlaps
                 gamma
                 use-air
                 
                 ;; Pass-through for any other options
                 additional-options)
  "Execute the BRL-CAD raytracer with MGED's current viewing parameters.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
  
  Keyword arguments:
    Output Control:
      :SIZE - Integer for square image (default 512), overridden by width/height
      :WIDTH - Integer pixel width (overrides :SIZE)
      :HEIGHT - Integer pixel height (overrides :SIZE)
      :OUTPUT-FILE - String path for output (.pix or .png)
      :FRAMEBUFFER - String framebuffer device (e.g., '/dev/Xl')
    
    View Control:
      :BACKGROUND-COLOR - Vector #(r g b) with values 0-255
      :PERSPECTIVE - Number in degrees (0-180)
      :AZIMUTH - Number in degrees (requires :ELEVATION, conflicts with -M)
      :ELEVATION - Number in degrees (requires :AZIMUTH, conflicts with -M)
      :ASPECT-RATIO - Number or string ratio (e.g., 1.33 or '4:3')
    
    Quality Control:
      :LIGHTING-MODEL - Integer 0-7 (default 0=full, 1=diffuse, 2=normals, etc.)
      :AMBIENT-LIGHT - Number 0.0-1.0 for ambient light intensity
      :HYPERSAMPLE - Integer for antialiasing (fires extra rays per pixel)
      :JITTER - Integer bit vector (1=randomize ray origin, 2=frame shift, 3=both)
      :PROCESSORS - Integer max processors to use
    
    Other:
      :BENCHMARK - Boolean, turns off random effects for reproducible results
      :REPORT-OVERLAPS - Boolean (default T unless :benchmark)
      :GAMMA - Number for gamma correction (e.g., 2.2)
      :USE-AIR - Boolean or integer to control air region rendering
      :ADDITIONAL-OPTIONS - List of additional rt option strings
  
  Returns:
    Result string from rt command"
  
  (let ((args '()))
    
    ;; Size/resolution options
    (cond
      ((and width height)
       (push "-w" args)
       (push (princ-to-string width) args)
       (push "-n" args)
       (push (princ-to-string height) args))
      (width
       (push "-w" args)
       (push (princ-to-string width) args)
       (push "-n" args)
       (push (princ-to-string width) args))  ; Square if only width given
      (height
       (push "-w" args)
       (push (princ-to-string height) args)
       (push "-n" args)
       (push (princ-to-string height) args))  ; Square if only height given
      (t
       (push "-s" args)
       (push (princ-to-string size) args)))
    
    ;; Output options
    (when output-file
      (push "-o" args)
      (push output-file args))
    
    (when framebuffer
      (push "-F" args)
      (push framebuffer args))
    
    ;; Background color
    (when background-color
      (push "-C" args)
      (push (color->string background-color) args))
    
    ;; View options
    (when perspective
      (push "-p" args)
      (push (princ-to-string perspective) args))
    
    (when (and azimuth elevation)
      (push "-a" args)
      (push (princ-to-string azimuth) args)
      (push "-e" args)
      (push (princ-to-string elevation) args))
    
    (when aspect-ratio
      (push "-V" args)
      (push (if (stringp aspect-ratio)
                aspect-ratio
                (princ-to-string aspect-ratio))
            args))
    
    ;; Quality options
    (when (/= lighting-model 0)
      (push "-l" args)
      (push (princ-to-string lighting-model) args))
    
    (when ambient-light
      (push "-A" args)
      (push (princ-to-string ambient-light) args))
    
    (when hypersample
      (push "-H" args)
      (push (princ-to-string hypersample) args))
    
    (when jitter
      (push "-J" args)
      (push (princ-to-string jitter) args))
    
    (when processors
      (push "-P" args)
      (push (princ-to-string processors) args))
    
    ;; Advanced options
    (when benchmark
      (push "-B" args))
    
    (when (and report-overlaps (not benchmark))
      (push "-r" args))
    
    (when (and (not report-overlaps) (not benchmark))
      (push "-R" args))
    
    (when gamma
      (push "-c" args)
      (push (format nil "set gamma=~A" gamma) args))
    
    (when use-air
      (push "-U" args)
      (push (princ-to-string (if (eq use-air t) 1 use-air)) args))
    
    ;; Additional options
    (when additional-options
      (setf args (append additional-options args)))
    
    ;; Objects to raytrace
    (let ((object-list (if objects
                          (ensure-list objects)
                          '())))
      (apply #'mged:rt (append (nreverse args) object-list)))))

(defun quick-preview (objects &key (framebuffer "/dev/Xl"))
  "Fast preview raytrace at low resolution.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
  
  Keyword arguments:
    :FRAMEBUFFER - String framebuffer device (default '/dev/Xl')
  
  Returns:
    Result string from rt command"
  (raytrace objects 
            :size 256 
            :framebuffer framebuffer))

(defun high-quality-render (objects output-file 
                           &key (width 1920) (height 1080) 
                                (hypersample 4))
  "High-quality render with antialiasing to file.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
    OUTPUT-FILE - String path for output (.pix or .png)
  
  Keyword arguments:
    :WIDTH - Integer pixel width (default 1920)
    :HEIGHT - Integer pixel height (default 1080)
    :HYPERSAMPLE - Integer antialiasing level (default 4)
  
  Returns:
    Result string from rt command"
  (raytrace objects
            :width width
            :height height
            :output-file output-file
            :hypersample hypersample
            :jitter 1))
