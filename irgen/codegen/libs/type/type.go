package types

import (
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/constant"
	"github.com/llir/llvm/ir/types"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/c"
	function "github.com/nagarajRPoojari/picasso/irgen/codegen/libs/func"
	typedef "github.com/nagarajRPoojari/picasso/irgen/codegen/type"
	bc "github.com/nagarajRPoojari/picasso/irgen/codegen/type/block"
	"github.com/nagarajRPoojari/picasso/irgen/codegen/type/primitives/ints"
)

type TypeHandler struct {
}

func NewTypeHandler() *TypeHandler {
	return &TypeHandler{}
}

func (t *TypeHandler) ListAllFuncs() map[string]function.Func {
	funcs := make(map[string]function.Func)
	funcs["size"] = t.Size
	funcs["type"] = t.TypeOf
	return funcs
}

func (t *TypeHandler) Size(_ *ir.Func, typeHandler *typedef.TypeHandler, module *ir.Module, bh *bc.BlockHolder, args []typedef.Var) typedef.Var {
	// assume args[0] holds the type or var we want sizeof
	typ := args[0].Type() // this should be `types.Type`

	// Create null pointer of given type
	nullPtr := constant.NewNull(types.NewPointer(typ))

	// GEP: move by 1 element
	gep := bh.N.NewGetElementPtr(typ, nullPtr, constant.NewInt(types.I32, 1))

	// Convert pointer to int (i64)
	sizeVal := bh.N.NewPtrToInt(gep, types.I64)
	slot := bh.V.NewAlloca(types.I64)
	bh.N.NewStore(sizeVal, slot)
	return &ints.Int64{
		NativeType: types.I64,
		Value:      slot,
	}
}

func (t *TypeHandler) TypeOf(_ *ir.Func, typeHandler *typedef.TypeHandler, module *ir.Module, bh *bc.BlockHolder, args []typedef.Var) typedef.Var {
	typ := args[0].NativeTypeString()

	strConst := constant.NewCharArrayFromString(typ)
	global := module.NewGlobalDef("", strConst)

	gep := bh.N.NewGetElementPtr(
		global.ContentType,
		global,
		constant.NewInt(types.I32, 0),
		constant.NewInt(types.I32, 0),
	)

	allocFn := c.Instance.Funcs[c.FUNC_STRING_ALLOC]
	s := bh.N.NewCall(allocFn, gep, constant.NewInt(types.I64, int64(len(typ))))
	return typedef.NewString(bh, s)
}
