package compiler.nodes

import compiler.Coder
import compiler.CompileException
import compiler.Opcode.*
import compiler.ValueType.*

abstract class N_UNOP(val arg: N_EXPRESSION): Node.N_EXPRESSION() {
    override fun setType() {
        super.setType()
        this.type = arg.type
    }
}
class N_NEGATE(arg: N_EXPRESSION): N_UNOP(arg) {
    override fun setType() {
        super.setType()
        this.type = arg.type
    }
    override fun code(coder: Coder) {
        arg.code(coder)
        when (this.type) {
            VAL_INT -> coder.code(OP_NEGI)
            VAL_FLOAT -> coder.code(OP_NEGF)
            VAL_VECTOR -> coder.code(OP_NEGV)
            else -> throw CompileException("type error: can only negate numeric values")
        }
    }
}
class N_INVERSE(arg: N_EXPRESSION): N_UNOP(arg) {
    override fun setType() {
        super.setType()
        if (arg.type != VAL_BOOL) throw CompileException("type error: can only NOT booleans")
        this.type = VAL_BOOL
    }
    override fun code(coder: Coder) {
        arg.code(coder)
        coder.code(OP_NOT)
    }
}