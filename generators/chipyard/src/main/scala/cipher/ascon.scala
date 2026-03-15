package chipyard.cipher

import chisel3._
import chisel3.util._
import chisel3.experimental.{IntParam, BaseModule}
import freechips.rocketchip.prci._
import freechips.rocketchip.subsystem.{BaseSubsystem, PBUS}
import org.chipsalliance.cde.config.{Parameters, Field, Config}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.regmapper.{HasRegMap, RegField}
import freechips.rocketchip.tilelink._

// DOC include start: Ascon params
case class ASCONParams(address: BigInt)
// DOC include end: Ascon params

// DOC include start: Ascon key
case object ASCONKey extends Field [Option[ASCONParams]](None)
// DOC include end: Ascon key

class ASCONIO extends Bundle {
  val i_clk           = Input(Clock())
  val i_rst           = Input(Bool())
  val i_write_data    = Input(Bool())
  val i_start         = Input(Bool())
  val out_fifo_rd_en  = Input(Bool())
  val input_finish    = Input(Bool())
  val i_data          = Input(UInt(32.W))
  val in_fifo_full    = Output(Bool())
  val out_fifo_empty  = Output(Bool())
  val o_done          = Output(Bool())
  val tag_valid       = Output(Bool())
  val out_fifo_data   = Output(UInt(32.W))
}

// declare register-map structure
object ASCONCtrlReg {
  val trigger     = 0x00
  val data_a      = 0x04 // data in
  val data_b      = 0x08 // data out
  val status      = 0x0C
}

// DOC include start: Ascon blackbox
class ascon extends BlackBox with HasBlackBoxResource {
  val io = IO(new ASCONIO)
  addResource("/vsrc/ascon/ascon.v")
  addResource("/vsrc/ascon/ascon_core.v")
  addResource("/vsrc/ascon/asconp.v")
  addResource("/vsrc/ascon/fifo.v")
}
// DOC include end: Ascon blackbox

// DOC include start: Ascon router
class ASCONTL(params: ASCONParams, beatBytes: Int)(implicit p: Parameters) extends ClockSinkDomain(ClockSinkParameters())(p) {
  val device = new SimpleDevice("ascon", Seq("deslab,ascon"))
  val node = TLRegisterNode(
    address = Seq(AddressSet(params.address, 0xfff)),
    device = device,
    deviceKey = "reg/control",
    beatBytes = beatBytes)

  override lazy val module = new ASCONImpl
  class ASCONImpl extends Impl {
    withClockAndReset(clock, reset) {
      // declare input
      val rst           = RegInit(false.B)
      val write_en      = WireInit(false.B) // trigger only one clock cycle
      val start         = WireInit(false.B) // trigger only one clock cycle
      val read_en       = WireInit(false.B) // trigger only one clock cycle
      val input_finish  = RegInit(false.B)
      val data_in       = Reg(UInt(32.W))

      // declare output
      val in_fifo_full  = Wire(Bool())
      val out_fifo_empty = Wire(Bool())
      val o_done        = Wire(Bool())
      val tag_valid     = Wire(Bool())
      val data_out      = Wire(UInt(32.W))

      // HW instantiation
      val impl = Module(new ascon)
      val impl_io = impl.io

      // mapping input
      impl_io.i_clk           := clock
      impl_io.i_rst           := reset.asBool || rst
      impl_io.i_write_data    := write_en
      impl_io.i_start         := start
      impl_io.out_fifo_rd_en  := read_en
      impl_io.input_finish    := input_finish
      impl_io.i_data          := data_in

      // mapping output
      in_fifo_full            := impl_io.in_fifo_full
      out_fifo_empty          := impl_io.out_fifo_empty
      o_done                  := impl_io.o_done
      tag_valid               := impl_io.tag_valid
      data_out                := impl_io.out_fifo_data

      // DOC include start: Ascon instance regmap
      node.regmap(
        ASCONCtrlReg.trigger -> Seq(
          RegField(1, start), // bit 0
          RegField(1, write_en),
          RegField(1, read_en),
          RegField(1, input_finish),
          RegField(1, rst)),
        ASCONCtrlReg.data_a -> Seq(RegField(32, data_in)),
        ASCONCtrlReg.data_b -> Seq(RegField.r(32, data_out)),
        ASCONCtrlReg.status -> Seq(
          RegField.r(1, in_fifo_full), // bit 0
          RegField.r(1, out_fifo_empty),
          RegField.r(1, tag_valid),
          RegField.r(1, o_done)))
      // DOC include end: Ascon instance regmap
    }
  }
}
// DOC include end: Ascon router

// DOC include start: Ascon lazy trait
trait CanHavePeripheralASCON { this: BaseSubsystem =>
  private val portName = "ascon"
  private val pbus = locateTLBusWrapper(PBUS)

  p(ASCONKey) match {
    case Some(params: ASCONParams) => {
      val ascon = {
        val ascon = LazyModule(new ASCONTL(params, pbus.beatBytes)(p))
        ascon.clockNode := pbus.fixedClockNode
        pbus.coupleTo(portName) {
          ascon.node := TLFragmenter(pbus.beatBytes, pbus.blockBytes) := _
        }
        ascon
      }
    }
    case None => None
  }
}
// DOC include end: Ascon lazy trait

// DOC include start: Ascon config fragment
class WithASCON(address: BigInt) extends Config((site, here, up) => {
  case ASCONKey => {
    Some(ASCONParams(address = address))
  }
})
// DOC include end: Ascon config fragment
