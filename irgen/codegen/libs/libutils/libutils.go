package libutils

import (
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/types"
	"github.com/llir/llvm/ir/value"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/handlers/utils"
	typedef "github.com/nagarajRPoojari/picasso/irgen/codegen/type"
	bc "github.com/nagarajRPoojari/picasso/irgen/codegen/type/block"
)

// CallCFunc handles FFI calls to C functions with support for:
// 1. Primitive types (pass by value)
// 2. Struct pointers (pass by reference - class instances)
// 3. Bare structs (pass/return by value - for multiple returns)
//
// Key behaviors:
// - Arguments: Converts class instances (struct pointers) to bare struct values when C expects struct by value
// - Returns: Wraps bare struct returns as Tuples for multiple return value support
func CallCFunc(typeHandler *typedef.TypeHandler, f *ir.Func, bh *bc.BlockHolder, args []typedef.Var) typedef.Var {
	castedArgs := make([]value.Value, 0)

	// Process arguments
	for i, arg := range args {
		if i >= len(f.Sig.Params) {
			castedArgs = append(castedArgs, arg.Load(bh))
			continue
		}

		expected := f.Sig.Params[i]
		argVal := arg.Load(bh)

		// Handle struct by value: if C expects bare struct but we have a pointer (class instance)
		// Load the struct value from the pointer
		if expectedStruct, ok := expected.(*types.StructType); ok {
			if ptrType, isPtrType := argVal.Type().(*types.PointerType); isPtrType {
				if _, isStructPtr := ptrType.ElemType.(*types.StructType); isStructPtr {
					// Load the struct value from the pointer
					argVal = bh.N.NewLoad(expectedStruct, argVal)
				}
			}
		}

		// Handle cross-module struct pointer mismatch: irgen names types (e.g. %string*) while
		// clang-generated FFI modules use full struct names (e.g. %struct.__public__string_t*).
		// They are the same layout but different LLVM type names across modules — bitcast via i8*
		// to avoid llvm-as type mismatch errors.
		if expPtr, expIsPtr := expected.(*types.PointerType); expIsPtr {
			if _, expElemIsStruct := expPtr.ElemType.(*types.StructType); expElemIsStruct {
				if argPtr, argIsPtr := argVal.Type().(*types.PointerType); argIsPtr {
					if _, argElemIsStruct := argPtr.ElemType.(*types.StructType); argElemIsStruct {
						if expected.String() != argVal.Type().String() {
							argVal = bh.N.NewBitCast(argVal, types.I8Ptr)
							argVal = bh.N.NewBitCast(argVal, expected)
						}
					}
				}
			}
		}

		// Apply implicit type casting
		argVal = typeHandler.ImplicitTypeCast(bh, utils.GetTypeString(expected), argVal)
		castedArgs = append(castedArgs, argVal)
	}

	result := bh.N.NewCall(f, castedArgs...)
	retType := f.Sig.RetType
	if retType == types.Void {
		return nil
	}

	// If return type is a bare struct (not a pointer), wrap it as a Tuple
	// This enables multiple return values via struct destructuring
	if structType, ok := retType.(*types.StructType); ok {
		// Extract type names from struct fields for tuple creation
		typeNames := extractTypeNamesFromStruct(structType)
		// Return Tuple directly - don't go through BuildVar as the struct type may not be registered
		return typedef.NewTupleFromStruct(bh, result, structType, typeNames)
	}

	// LLVM sometimes represents small structs as arrays (e.g., {i64, i64} becomes [2 x i64])
	// Check if this is an array that should be treated as a struct for multiple returns
	if arrayType, ok := retType.(*types.ArrayType); ok {
		// For arrays, we need to allocate space and store the array, then extract elements
		// This is because extractvalue works differently for arrays vs structs

		// Allocate space for the array
		arraySlot := bh.N.NewAlloca(arrayType)
		bh.N.NewStore(result, arraySlot)

		// Extract type names
		typeNames := make([]string, arrayType.Len)
		for i := uint64(0); i < arrayType.Len; i++ {
			typeNames[i] = utils.GetTypeString(arrayType.ElemType)
		}

		// Create a tuple-like wrapper for the array
		// We'll use a custom array tuple type
		return typedef.NewTupleFromArray(bh, result, arrayType, typeNames)
	}

	// For pointer types (including struct pointers), check if it's a registered class
	if ptrType, ok := retType.(*types.PointerType); ok {
		if _, ok := ptrType.ElemType.(*types.StructType); ok {
			// This is a struct pointer - try to build as a class
			typeName := utils.GetTypeString(retType)
			return typeHandler.BuildVar(bh, typedef.NewType(typeName), result)
		}
	}

	// For all other types (primitives, simple pointers), build a regular Var
	return typeHandler.BuildVar(bh, typedef.NewType(utils.GetTypeString(result.Type())), result)
}

// extractTypeNamesFromStruct extracts type names from a struct's fields
// This is used to create proper type information for Tuple wrapping
func extractTypeNamesFromStruct(structType *types.StructType) []string {
	typeNames := make([]string, len(structType.Fields))
	for i, field := range structType.Fields {
		typeNames[i] = utils.GetTypeString(field)
	}
	return typeNames
}
