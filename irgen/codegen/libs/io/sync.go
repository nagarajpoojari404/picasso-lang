package io

import (
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/types"
	"github.com/llir/llvm/ir/value"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/handlers/utils"

	function "github.com/nagarajRPoojari/picasso/irgen/codegen/libs/func"
	typedef "github.com/nagarajRPoojari/picasso/irgen/codegen/type"
	bc "github.com/nagarajRPoojari/picasso/irgen/codegen/type/block"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/type/primitives/floats"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/type/primitives/ints"
)

const (
	ALIAS_SPRINTF = "printf"
	ALIAS_SSCANF  = "scanf"
)

type SyncIO struct {
}

func NewSyncIO() *SyncIO {
	return &SyncIO{}
}

func (t *SyncIO) ListAllFuncs() map[string]function.Func {
	funcs := make(map[string]function.Func)
	funcs[ALIAS_SPRINTF] = t.sprintf
	funcs[ALIAS_SSCANF] = t.sscanf
	return funcs
}

func (t *SyncIO) sprintf(f *ir.Func, typeHandler *typedef.TypeHandler, module *ir.Module, bh *bc.BlockHolder, args []typedef.Var) typedef.Var {
	// Format string is %string* in irgen but FFI expects %struct.__public__string_t* — bitcast via i8*
	fmtVal := args[0].Load(bh)
	if f != nil && len(f.Sig.Params) > 0 {
		if expected := f.Sig.Params[0]; expected.String() != fmtVal.Type().String() {
			fmtVal = bh.N.NewBitCast(fmtVal, types.I8Ptr)
			fmtVal = bh.N.NewBitCast(fmtVal, expected)
		}
	}
	castedArgs := []value.Value{fmtVal} // The Format String

	for _, arg := range args[1:] {
		val := arg.Load(bh)
		if _, ok := val.Type().(*types.PointerType); ok {
			castedArgs = append(castedArgs, bh.N.NewBitCast(val, types.I8Ptr))
			continue
		}
		switch arg.(type) {
		case *ints.Int8, *ints.Int16, *ints.Int32:
			res := typeHandler.ImplicitIntCast(bh, val, types.I32)
			castedArgs = append(castedArgs, res)
		case *ints.Int64:
			castedArgs = append(castedArgs, val)
		case *ints.UInt8, *ints.UInt16:
			res := typeHandler.ImplicitUnsignedIntCast(bh, val, types.I32)
			castedArgs = append(castedArgs, res)
		case *ints.UInt32:
			res := typeHandler.ImplicitUnsignedIntCast(bh, val, types.I64)
			castedArgs = append(castedArgs, res)
		case *ints.UInt64:
			castedArgs = append(castedArgs, val)
		case *floats.Float16, *floats.Float32, *floats.Float64:
			res := typeHandler.ImplicitFloatCast(bh, val, types.Double)
			castedArgs = append(castedArgs, res)
		default:
			castedArgs = append(castedArgs, val)
		}
	}
	result := bh.N.NewCall(f, castedArgs...)
	return typeHandler.BuildVar(bh, typedef.NewType(utils.GetTypeString(result.Type())), result)
}

func (t *SyncIO) sscanf(f *ir.Func, typeHandler *typedef.TypeHandler, module *ir.Module, bh *bc.BlockHolder, args []typedef.Var) typedef.Var {
	format := args[0].Load(bh)
	callArgs := []value.Value{format}

	for _, arg := range args[1:] {
		var slot value.Value
		switch arg.NativeTypeString() {
		case "string":
			slot = arg.Load(bh)
		default:
			slot = arg.Slot()
		}

		callArgs = append(callArgs, slot)
	}

	result := bh.N.NewCall(f, callArgs...)
	return typeHandler.BuildVar(bh, typedef.NewType(utils.GetTypeString(result.Type())), result)
}
