package myrom

import chisel3._
import chisel3.util.HasBlackBoxResource
import freechips.rocketchip.tile.{LazyRoCC, LazyRoCCModuleImp, OpcodeSet, RoCCIO}
import org.chipsalliance.cde.config.Parameters


class myromIO extends Bundle {
  val clk = Input(Clock())
  val addr = Input(UInt(3.W))
  val dout = Output(UInt(32.W))
}

class rom_8x32 extends BlackBox with HasBlackBoxResource {
  val io = IO(new myromIO)
  addResource("/vsrc/rom_8x32.v")
}

class MyRoCC (opcodes: OpcodeSet)(implicit p: Parameters) extends LazyRoCC(opcodes) {
  override lazy val module = new MyRoCCModule(this)
}

class MyRoCCModule(outer: MyRoCC) extends LazyRoCCModuleImp(outer) {
  // Instantiate the rocc modules
  val myRoCCBlackBox    = Module(new rom_8x32)

  // Connect
}