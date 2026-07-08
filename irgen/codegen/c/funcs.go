package c

import (
	"github.com/llir/llvm/ir"
	"github.com/llir/llvm/ir/types"
)

// registerFuncs orchestrates the declaration of all external symbols within the
// current LLVM module. It partitions declarations into logical runtime domains.
func (t *Interface) registerFuncs(mod *ir.Module) {
	t.initRuntime(mod) // Core lifecycle and memory
}

// initRuntime declares core engine functions including memory management (malloc/alloc),
// threading, process synchronization, and essential libc utilities like memcpy.
func (t *Interface) initRuntime(mod *ir.Module) {

	t.Funcs[FUNC_MEMCPY] = mod.NewFunc("llvm.memcpy.p0i8.p0i8.i64",
		types.Void,
		ir.NewParam("dest", types.I8Ptr),
		ir.NewParam("src", types.I8Ptr),
		ir.NewParam("len", types.I64),
		ir.NewParam("isvolatile", types.I1),
	)
	// @thread
	fnType := types.NewFunc(
		types.NewPointer(types.I8),
		types.NewPointer(types.I8),
	)
	t.Funcs[FUNC_THREAD] = mod.NewFunc(FUNC_THREAD, types.Void,
		ir.NewParam("", types.NewPointer(fnType)),
		ir.NewParam("", types.I32),
	)
	t.Funcs[FUNC_THREAD].Sig.Variadic = true

	// @self_yield
	t.Funcs[FUNC_SELF_YIELD] = mod.NewFunc(FUNC_SELF_YIELD, types.Void)

	// @alloc
	t.Funcs[FUNC_ALLOC] = mod.NewFunc(FUNC_ALLOC, types.I8Ptr, ir.NewParam("", types.I64))

	// @runtime_init
	t.Funcs[FUNC_RUNTIME_INIT] = mod.NewFunc(FUNC_RUNTIME_INIT, types.Void)

	// @array_alloc
	t.Funcs[FUNC_ARRAY_ALLOC] = mod.NewFunc(FUNC_ARRAY_ALLOC, types.NewPointer(t.Types[TYPE_ARRAY]), ir.NewParam("", types.I32), ir.NewParam("", types.I32))
	t.Funcs[FUNC_ARRAY_ALLOC].Sig.Variadic = true

	// @extend_array
	t.Funcs[FUNC_EXTEND_ARRAY] = mod.NewFunc(FUNC_EXTEND_ARRAY, types.Void, ir.NewParam("", types.NewPointer(t.Types[TYPE_ARRAY])), ir.NewParam("", types.I32))

	// @get_subarray
	t.Funcs[FUNC_GET_SUBARRAY] = mod.NewFunc(FUNC_GET_SUBARRAY, types.NewPointer(t.Types[TYPE_ARRAY]),
		ir.NewParam("arr", types.NewPointer(t.Types[TYPE_ARRAY])),
		ir.NewParam("index", types.I64))

	// @set_subarray
	t.Funcs[FUNC_SET_SUBARRAY] = mod.NewFunc(FUNC_SET_SUBARRAY, types.Void,
		ir.NewParam("arr", types.NewPointer(t.Types[TYPE_ARRAY])),
		ir.NewParam("index", types.I64),
		ir.NewParam("sub_arr", types.NewPointer(t.Types[TYPE_ARRAY])))

	// @string_alloc — (const char* fmt, size_t size) -> string*
	t.Funcs[FUNC_STRING_ALLOC] = mod.NewFunc(FUNC_STRING_ALLOC, types.NewPointer(t.Types[TYPE_STRING]),
		ir.NewParam("fmt", types.NewPointer(types.I8)),
		ir.NewParam("size", types.I64))
	t.Funcs[FUNC_STRING_SUBSTRING] = mod.NewFunc(FUNC_STRING_SUBSTRING, types.NewPointer(t.Types[TYPE_STRING]), ir.NewParam("", types.NewPointer(t.Types[TYPE_STRING])), ir.NewParam("", types.I64), ir.NewParam("", types.I64))
	t.Funcs[FUNC_STRING_FORMAT] = mod.NewFunc(FUNC_STRING_FORMAT, types.NewPointer(t.Types[TYPE_STRING]), ir.NewParam("", types.NewPointer(t.Types[TYPE_STRING])))
	t.Funcs[FUNC_STRING_FORMAT].Sig.Variadic = true

	t.Funcs[__UTILS__FUNC_DEBUG_ARRAY_INFO] = mod.NewFunc(
		__UTILS__FUNC_DEBUG_ARRAY_INFO,
		types.Void, ir.NewParam("", types.NewPointer(t.Types[TYPE_ARRAY])),
	)
}
