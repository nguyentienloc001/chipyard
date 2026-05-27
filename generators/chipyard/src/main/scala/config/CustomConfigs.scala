package chipyard

import org.chipsalliance.cde.config._
import freechips.rocketchip.subsystem.{MBUS, SBUS}
import constellation.channel._
import constellation.routing._
import constellation.router._
import constellation.topology._
import constellation.noc._

import scala.collection.immutable.ListMap

// DEPRECATED: Previously used by CustomSoC. Crypto accelerators (AES, SHA3, ChaCha)
// sit on pbus, not sbus — they do not exercise the interconnect under test.
// Kept for reference only; not instantiated by any active config.
class PeripheralConfig extends Config(
//  new chipyard.cipher.WithMyTimer(address = 0x1000E000) ++
  new chipyard.cipher.WithAES(address = 0x10008000) ++
  new chipyard.cipher.WithSHA3(address = 0x10007000) ++
//  new chipyard.cipher.WithPrince(address = 0x1000B000) ++
//  new chipyard.cipher.WithBLAKE2S(address = 0x1000A000) ++
//  new chipyard.cipher.WithDMPresent(address = 0x10009000) ++
//  new chipyard.cipher.WithPresent(address = 0x10008000) ++
  new chipyard.cipher.WithChaCha(address = 0x10006000)
//  new chipyard.cipher.WithKLEIN(address = 0x10006000) ++
)

// DEPRECATED: Legacy config with crypto accelerators, kept for reference only
class GCDTLBlackBoxRocketConfig extends Config(
  new chipyard.cipher.WithMyTimer(address = 0x1000E000) ++
//    new chipyard.cipher.WithAES(address = 0x1000D000) ++
//    new chipyard.cipher.WithSHA3(address = 0x1000C000) ++
//    new chipyard.cipher.WithPrince(address = 0x1000B000) ++
    new chipyard.cipher.WithBLAKE2S(address = 0x1000A000) ++
//    new chipyard.cipher.WithDMPresent(address = 0x10009000) ++
//    new chipyard.cipher.WithPresent(address = 0x10008000) ++
    new chipyard.cipher.WithChaCha(address = 0x10007000) ++
//    new chipyard.cipher.WithKLEIN(address = 0x10006000) ++
    new chipyard.cipher.WithASCON(address = 0x10006000) ++
    new freechips.rocketchip.subsystem.WithoutTLMonitors ++
    new freechips.rocketchip.rocket.WithNBigCores(1) ++
    new chipyard.config.AbstractConfig)

class CustomSoC extends Config(
  new freechips.rocketchip.subsystem.WithoutTLMonitors ++
  new chipyard.config.WithNoUART ++
  new testchipip.soc.WithNoScratchpads ++
  new freechips.rocketchip.rocket.WithNBigCores(4) ++
  new chipyard.config.AbstractConfig
)

class ThesisSoC extends Config(
  new chipyard.example.WithBusUtilMonitor ++
  new freechips.rocketchip.subsystem.WithoutTLMonitors ++
  new chipyard.config.WithUART(address = 0x64000000) ++
  new chipyard.config.WithNoUART ++
  new testchipip.soc.WithNoScratchpads ++
  new freechips.rocketchip.rocket.WithNBigCores(4) ++
  new chipyard.config.AbstractConfig
)

// Shared bus baseline — broadcast coherence manager replaces crossbar directory protocol
// Worst-case interconnect: all requests serialized through a single broadcast.
// Lower-bound for all interconnect comparisons.
class SharedBusSoC extends Config(
  new freechips.rocketchip.subsystem.WithoutTLMonitors ++
  new chipyard.config.WithBroadcastManager ++
  new chipyard.config.WithUART(address = 0x64000000) ++
  new chipyard.config.WithNoUART ++
  new testchipip.soc.WithNoScratchpads ++
  new freechips.rocketchip.rocket.WithNBigCores(4) ++
  new chipyard.config.AbstractConfig
)


// DOC include start: QuadCoreRing
class QuadCoreRing extends Config(
  new constellation.soc.WithSbusNoC(constellation.protocol.SimpleTLNoCParams(
    constellation.protocol.DiplomaticNetworkNodeMapping(
      inNodeMapping = ListMap(
        "Core 0" -> 0, "Core 1" -> 1,
        "Core 2" -> 2, "Core 3" -> 3,
        "debug[0]" -> 4),
      outNodeMapping = ListMap(
        "system[0]" -> 5,
        "pbus" -> 4)),
    nocParams = NoCParams(
      topology = BidirectionalTorus1D(6),
      channelParamGen = (a, b) => UserChannelParams(Seq.fill(10) { UserVirtualChannelParams(4) }),
      routingRelation = NonblockingVirtualSubnetworksRouting(BidirectionalTorus1DShortestRouting(), 5, 2))
  )) ++
  new ThesisSoC ++
  new chipyard.config.AbstractConfig
)
// DOC include end: QuadCoreRing


// DOC include start: QuadCoreMeshThesis
class QuadCoreMeshThesis extends Config(
  new constellation.soc.WithSbusNoC(constellation.protocol.SimpleTLNoCParams(
    constellation.protocol.DiplomaticNetworkNodeMapping(
      inNodeMapping = ListMap(
        "Core 0" -> 0, "Core 1" -> 1,
        "Core 2" -> 2, "Core 3" -> 3,
        "debug[0]" -> 5),
      outNodeMapping = ListMap(
        "system[0]" -> 4,
        "pbus" -> 5)),
    nocParams = NoCParams(
      topology = Mesh2D(nX = 3, nY = 2),
      channelParamGen = (a, b) => UserChannelParams(Seq.fill(10) { UserVirtualChannelParams(4) }),
      routingRelation = NonblockingVirtualSubnetworksRouting(Mesh2DDimensionOrderedRouting(), 5, 2))
  )) ++
  new ThesisSoC ++
  new chipyard.config.AbstractConfig
)
// DOC include end: QuadCoreMeshThesis

// DOC include start: QuadCoreMesh
class QuadCoreMesh extends Config(
  new constellation.soc.WithSbusNoC(constellation.protocol.SimpleTLNoCParams(
    constellation.protocol.DiplomaticNetworkNodeMapping(
      inNodeMapping = ListMap(
        "Core 0" -> 0, "Core 1" -> 1,
        "Core 2" -> 2, "Core 3" -> 3,
        "debug[0]" -> 5),
      outNodeMapping = ListMap(
        "system[0]" -> 4,
        "pbus" -> 5)),
    nocParams = NoCParams(
      topology = Mesh2D(nX = 3, nY = 2),
      channelParamGen = (a, b) => UserChannelParams(Seq.fill(10) {UserVirtualChannelParams(4) }),
      routingRelation = NonblockingVirtualSubnetworksRouting(Mesh2DDimensionOrderedRouting(), 5, 2))
  )) ++
  new ThesisSoC ++
  new chipyard.config.AbstractConfig
)
// DOC include end: QuadCoreMesh

// DOC include start: QuadCoreTree
// Tree topology NoC — BidirectionalTree(height=2, dAry=2) = 7 routers
// Node layout: root=0, level-1={1,2}, leaves={3,4,5,6}
// Cores at leaves (nodes 3-6), memory/system at root (node 0).
// Natural hierarchy matches NUMA-like memory access patterns.
class QuadCoreTree extends Config(
  new constellation.soc.WithSbusNoC(constellation.protocol.SimpleTLNoCParams(
    constellation.protocol.DiplomaticNetworkNodeMapping(
      inNodeMapping = ListMap(
        "Core 0" -> 3, "Core 1" -> 4,
        "Core 2" -> 5, "Core 3" -> 6,
        "debug[0]" -> 0),
      outNodeMapping = ListMap(
        "system[0]" -> 0,
        "pbus" -> 0)),
    nocParams = NoCParams(
      topology = BidirectionalTree(height = 2, dAry = 2),
      channelParamGen = (a, b) => UserChannelParams(Seq.fill(10) { UserVirtualChannelParams(4) }),
      routingRelation = NonblockingVirtualSubnetworksRouting(BidirectionalTreeRouting(), 5, 2))
  )) ++
  new ThesisSoC ++
  new chipyard.config.AbstractConfig
)
// DOC include end: QuadCoreTree