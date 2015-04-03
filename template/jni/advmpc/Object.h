#pragma once

/*
 * There are three types of objects:
 *  Class objects - an instance of java.lang.Class
 *  Array objects - an object created with a "new array" instruction
 *  Data objects - an object that is neither of the above
 *
 * We also define String objects.  At present they're equivalent to
 * DataObject, but that may change.  (Either way, they make some of the
 * code more obvious.)
 *
 * All objects have an Object header followed by type-specific data.
 */
struct Object {
    /* ptr to class object */
    /*ClassObject**/void*    clazz;

    /*
     * A word containing either a "thin" lock or a "fat" monitor.  See
     * the comments in Sync.c for a description of its layout.
     */
    u4              lock;
};

/*
 * Array objects have these additional fields.
 *
 * We don't currently store the size of each element.  Usually it's implied
 * by the instruction.  If necessary, the width can be derived from
 * the first char of obj->clazz->descriptor.
 */
struct ArrayObject : Object {
    /* number of elements; immutable after init */
    u4              length;

    /*
     * Array contents; actual size is (length * sizeof(type)).  This is
     * declared as u8 so that the compiler inserts any necessary padding
     * (e.g. for EABI); the actual allocation may be smaller than 8 bytes.
     */
    u8              contents[1];
};