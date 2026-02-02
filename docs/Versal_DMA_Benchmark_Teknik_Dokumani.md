# Versal DMA Benchmark Projesi
## Teknik Mimari ve Tasarım Dokümantasyonu

**Versiyon:** 1.0
**Tarih:** Şubat 2026
**Platform:** AMD/Xilinx VPK120 Evaluation Kit
**Hedef Cihaz:** Versal Premium VP1202

---

## İçindekiler

1. [Giriş ve Proje Amacı](#1-giriş-ve-proje-amacı)
2. [Versal Mimarisi Genel Bakış](#2-versal-mimarisi-genel-bakış)
3. [VPK120 Geliştirme Kartı](#3-vpk120-geliştirme-kartı)
4. [DMA Türleri ve Karakteristikleri](#4-dma-türleri-ve-karakteristikleri)
5. [Bellek Hiyerarşisi ve Adres Haritası](#5-bellek-hiyerarşisi-ve-adres-haritası)
6. [Network-on-Chip (NoC) Mimarisi](#6-network-on-chip-noc-mimarisi)
7. [Sistem Tasarım Mimarisi](#7-sistem-tasarım-mimarisi)
8. [LPD DMA ve DDR Erişim Problemi](#8-lpd-dma-ve-ddr-erişim-problemi)
9. [Benchmark Metodolojisi](#9-benchmark-metodolojisi)
10. [Performans Metrikleri ve Beklentiler](#10-performans-metrikleri-ve-beklentiler)
11. [Tasarım Kararları ve Trade-off'lar](#11-tasarım-kararları-ve-trade-offlar)
12. [Sonuç ve Gelecek Çalışmalar](#12-sonuç-ve-gelecek-çalışmalar)

---

## 1. Giriş ve Proje Amacı

### 1.1 Projenin Kapsamı

Bu proje, AMD/Xilinx Versal Premium serisi FPGA'ler üzerinde bulunan farklı DMA (Direct Memory Access) mekanizmalarının kapsamlı bir performans karşılaştırmasını gerçekleştirmek amacıyla tasarlanmıştır. Versal mimarisi, önceki nesil FPGA'lerden farklı olarak heterojen bir işlem platformu sunmakta ve bu platform üzerinde birden fazla DMA türü bulunmaktadır.

### 1.2 Motivasyon

Modern FPGA tabanlı sistemlerde veri hareketi, toplam sistem performansının en kritik belirleyicilerinden biridir. Özellikle:

- **Yüksek Bant Genişliği Uygulamaları:** Video işleme, radar sinyal işleme, 5G baseband
- **Düşük Gecikme Gereksinimleri:** Finansal işlemler (HFT), endüstriyel kontrol sistemleri
- **Enerji Verimliliği:** Gömülü sistemler, edge computing uygulamaları

Bu uygulamalarda doğru DMA mekanizmasının seçilmesi, sistem performansını %50'ye varan oranlarda etkileyebilmektedir.

### 1.3 Hedefler

1. Versal platformundaki tüm DMA türlerinin throughput ve latency karakterizasyonu
2. Farklı bellek türleri arasındaki transfer performansının ölçülmesi
3. DMA modlarının (Simple vs Scatter-Gather, Polling vs Interrupt) karşılaştırılması
4. Uygulama senaryolarına göre en uygun DMA seçimi için rehber oluşturulması

---

## 2. Versal Mimarisi Genel Bakış

### 2.1 Versal ACAP Kavramı

Versal, AMD/Xilinx'in "Adaptive Compute Acceleration Platform" (ACAP) olarak tanımladığı yeni nesil bir mimaridir. Geleneksel FPGA'lerden farklı olarak, Versal heterojen bir işlem platformu sunar:

```
┌─────────────────────────────────────────────────────────────────┐
│                        VERSAL ACAP                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐       │
│  │   Scalar      │  │  Adaptable    │  │  Intelligent  │       │
│  │   Engines     │  │   Engines     │  │   Engines     │       │
│  │   (PS)        │  │   (PL)        │  │   (AI Engine) │       │
│  │               │  │               │  │               │       │
│  │ • ARM Cortex  │  │ • LUTs        │  │ • VLIW Vector │       │
│  │   A72 (APU)   │  │ • FFs         │  │   Processors  │       │
│  │ • ARM Cortex  │  │ • DSPs        │  │ • 400+ Tiles  │       │
│  │   R5F (RPU)   │  │ • BRAMs       │  │ • 32 TOPS     │       │
│  │ • Platform    │  │ • URAMs       │  │               │       │
│  │   Management  │  │               │  │               │       │
│  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘       │
│          │                  │                  │                │
│          └──────────────────┼──────────────────┘                │
│                             │                                   │
│                    ┌────────┴────────┐                         │
│                    │  Network-on-Chip │                         │
│                    │      (NoC)       │                         │
│                    └────────┬────────┘                         │
│                             │                                   │
│          ┌──────────────────┼──────────────────┐               │
│          │                  │                  │                │
│    ┌─────┴─────┐     ┌─────┴─────┐     ┌─────┴─────┐          │
│    │   DDR     │     │  LPDDR    │     │   HBM     │          │
│    │Controller │     │Controller │     │Controller │          │
│    └───────────┘     └───────────┘     └───────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Processing System (PS) Detayları

Versal PS, aşağıdaki bileşenlerden oluşur:

#### 2.2.1 Application Processing Unit (APU)
- **İşlemci:** Dual-core ARM Cortex-A72 (64-bit)
- **Frekans:** 1.7 GHz'e kadar
- **Cache:**
  - L1: 32KB I-Cache + 32KB D-Cache (per core)
  - L2: 1MB shared
- **Özellikler:** ARMv8-A, NEON SIMD, Crypto extensions

#### 2.2.2 Real-time Processing Unit (RPU)
- **İşlemci:** Dual-core ARM Cortex-R5F (32-bit)
- **Frekans:** 600 MHz'e kadar
- **TCM:** 256KB (Tightly Coupled Memory)
- **Kullanım:** Gerçek zamanlı kontrol, düşük gecikme işlemleri

#### 2.2.3 Platform Management Controller (PMC)
- **İşlemci:** Triple-redundant MicroBlaze
- **Görevler:** Boot, güvenlik, güç yönetimi, konfigürasyon

### 2.3 Processing System Domainleri

Versal PS iki ana domain'e ayrılır:

| Domain | Kısaltma | Özellikler | DMA |
|--------|----------|------------|-----|
| **Full Power Domain** | FPD | APU, yüksek performans | FPD DMA (GDMA) |
| **Low Power Domain** | LPD | RPU, düşük güç | LPD DMA (ADMA) |

Bu ayrım, güç yönetimi ve izolasyon açısından kritik öneme sahiptir. LPD, FPD kapatıldığında bile çalışmaya devam edebilir.

---

## 3. VPK120 Geliştirme Kartı

### 3.1 Kart Özellikleri

VPK120, Versal Premium VP1202 cihazını barındıran bir değerlendirme kartıdır:

| Özellik | Değer |
|---------|-------|
| **FPGA** | xcvp1202-vsva2785-2MP-e-S |
| **Seri** | Versal Premium |
| **Speed Grade** | -2MP (Medium Performance) |
| **Temperature** | Extended (-40°C to +100°C) |

### 3.2 Bellek Kaynakları

#### 3.2.1 Off-chip Bellek

| Bellek Türü | Kapasite | Interface | Bant Genişliği |
|-------------|----------|-----------|----------------|
| **LPDDR4** | 2 GB | 32-bit, 4266 MT/s | ~17 GB/s |
| **DDR4 SODIMM** | 8 GB (opsiyonel) | 64-bit, 3200 MT/s | ~25 GB/s |

#### 3.2.2 On-chip Bellek

| Bellek Türü | Kapasite | Erişim Gecikmesi | Kullanım |
|-------------|----------|------------------|----------|
| **OCM** | 256 KB | ~2-3 cycle | Düşük gecikme buffer |
| **TCM** | 256 KB | 1 cycle | RPU özel bellek |
| **PL BRAM** | ~32 Mb | 1-2 cycle | PL yerel depolama |
| **PL URAM** | ~65 Mb | 1 cycle | Yüksek yoğunluk depolama |

### 3.3 Bağlantı Seçenekleri

- **PCIe:** Gen4 x8 (CPM üzerinden)
- **Ethernet:** 10/25/100G (GT transceivers)
- **USB:** USB 2.0/3.0
- **UART:** Debug ve konsol
- **JTAG:** Programlama ve debug

---

## 4. DMA Türleri ve Karakteristikleri

### 4.1 DMA Sınıflandırması

Versal platformunda DMA'lar iki ana kategoride incelenir:

```
                        VERSAL DMA TÜRLERİ
                              │
            ┌─────────────────┴─────────────────┐
            │                                   │
     PS Hard IP DMA                      PL Soft IP DMA
            │                                   │
    ┌───────┴───────┐               ┌──────────┼──────────┐
    │               │               │          │          │
 LPD DMA        FPD DMA         AXI DMA   AXI CDMA   AXI MCDMA
 (ADMA)         (GDMA)
```

### 4.2 LPD DMA (ADMA) - Low Power Domain DMA

#### 4.2.1 Mimari Özellikleri

LPD DMA, Versal PS içinde yer alan 8 kanallı bir hard IP DMA controller'dır. "ADMA" (Advanced DMA) veya "ZDMA" (Zynq DMA) olarak da bilinir.

**Temel Özellikler:**
- **Kanal Sayısı:** 8 bağımsız kanal
- **Veri Genişliği:** 128-bit
- **Adres Genişliği:** 44-bit (16 TB adreslenebilir)
- **Burst Desteği:** AXI4 burst (256 beat'e kadar)
- **Descriptor Tabanlı:** Linked-list descriptor yapısı

#### 4.2.2 Çalışma Modları

| Mod | Açıklama | Kullanım Senaryosu |
|-----|----------|-------------------|
| **Simple Mode** | Tek transfer, register tabanlı | Küçük, tek seferlik transferler |
| **Scatter-Gather** | Descriptor chain | Büyük, parçalı veri transferleri |
| **Linear Mode** | Ardışık bellek erişimi | Streaming uygulamaları |

#### 4.2.3 Güç Yönetimi Avantajı

LPD DMA, Low Power Domain'de bulunması sayesinde:
- FPD kapatıldığında bile çalışabilir
- Düşük güç modlarında aktif kalabilir
- Wake-up event'leri üretebilir

### 4.3 AXI DMA - Stream DMA

#### 4.3.1 Mimari

AXI DMA, memory-mapped ve stream interface'ler arasında veri transferi sağlayan bir soft IP'dir.

```
                    ┌─────────────────────────────┐
                    │         AXI DMA             │
                    │                             │
    AXI4 MM ◄───────┤  MM2S    ┌────┐    S2MM   ├───────► AXI4 MM
    (Memory)        │  Engine  │FIFO│   Engine   │         (Memory)
                    │    │     └────┘      │     │
                    │    ▼                 ▲     │
                    │ ┌────┐           ┌────┐   │
                    │ │M_AXIS          │S_AXIS  │
                    └─┴────┴───────────┴────┴───┘
                         │                 ▲
                         ▼                 │
                    Stream Peripheral (örn: FIFO, IP Core)
```

#### 4.3.2 Kanal Yapısı

| Kanal | Yön | Açıklama |
|-------|-----|----------|
| **MM2S** | Memory → Stream | Bellekten okuyup stream olarak çıkış |
| **S2MM** | Stream → Memory | Stream girişi alıp belleğe yazma |
| **SG** | Bidirectional | Scatter-Gather descriptor erişimi |

#### 4.3.3 Buffer Length Register

Bu projede kritik bir parametre olan Buffer Length Register, maksimum tek seferde transfer edilebilecek veri miktarını belirler:

| Bit Genişliği | Maksimum Transfer | Kullanım |
|---------------|-------------------|----------|
| 14-bit | 16 KB | Varsayılan, küçük transferler |
| 23-bit | 8 MB | Orta ölçekli |
| **26-bit** | **64 MB** | Bu projede kullanılan, büyük transferler |

### 4.4 AXI CDMA - Central DMA

#### 4.4.1 Temel Fark

AXI CDMA, stream interface'i olmayan, tamamen memory-to-memory transfer odaklı bir DMA'dır:

```
    ┌─────────────────────────────────────────┐
    │              AXI CDMA                   │
    │                                         │
    │   Source         ┌────────┐   Dest      │
    │   Address ──────►│  DMA   │◄───── Address
    │                  │ Engine │             │
    │   AXI4 MM ◄─────►│        │◄────► AXI4 MM
    │   (Read)         └────────┘       (Write)│
    └─────────────────────────────────────────┘
```

#### 4.4.2 Kullanım Senaryoları

- Bellek kopyalama (memcpy alternatifi)
- Buffer reorganizasyonu
- DMA-to-DMA veri hareketi
- Cache bypass gerektiren transferler

#### 4.4.3 Performans Karakteristikleri

CDMA, stream overhead'i olmadığı için memory-to-memory transferlerde genellikle en yüksek throughput'u sağlar.

### 4.5 AXI MCDMA - Multi-Channel DMA

#### 4.5.1 Çok Kanallı Mimari

MCDMA, 16'ya kadar bağımsız MM2S ve S2MM kanalı destekler:

```
    ┌─────────────────────────────────────────────────┐
    │                 AXI MCDMA                       │
    │                                                 │
    │  ┌─────────┐                    ┌─────────┐    │
    │  │ MM2S    │   Shared           │ S2MM    │    │
    │  │ Ch 0-15 │◄──AXI MM──►        │ Ch 0-15 │    │
    │  │         │   Interface        │         │    │
    │  └────┬────┘                    └────┬────┘    │
    │       │                              │         │
    │  ┌────┴────┐                    ┌────┴────┐    │
    │  │Scheduler│                    │Scheduler│    │
    │  │(RR/Pri) │                    │(RR/Pri) │    │
    │  └────┬────┘                    └────┬────┘    │
    │       │                              │         │
    │  M_AXIS[15:0]                  S_AXIS[15:0]    │
    └───────┴──────────────────────────────┴────────┘
```

#### 4.5.2 Scheduling Modları

| Mod | Açıklama | Kullanım |
|-----|----------|----------|
| **Round-Robin** | Her kanala eşit öncelik | Dengeli bant genişliği dağılımı |
| **Strict Priority** | Düşük numaralı kanal öncelikli | QoS gerektiren uygulamalar |

#### 4.5.3 Tipik Kullanım Senaryoları

- Multi-stream video işleme
- Çoklu sensör veri toplama
- Network packet processing
- Multi-channel audio

### 4.6 DMA Karşılaştırma Tablosu

| Özellik | LPD DMA | AXI DMA | AXI CDMA | AXI MCDMA |
|---------|---------|---------|----------|-----------|
| **Tip** | Hard IP | Soft IP | Soft IP | Soft IP |
| **Kanal** | 8 | 1 (MM2S+S2MM) | 1 | 16+16 |
| **Interface** | MM only | MM + Stream | MM only | MM + Stream |
| **SG Desteği** | Evet | Evet | Evet | Dahili |
| **Max Veri Gen.** | 128-bit | 1024-bit | 1024-bit | 512-bit |
| **PL Kaynak** | Yok | Orta | Düşük | Yüksek |
| **Güç** | Düşük | Orta | Düşük | Yüksek |

---

## 5. Bellek Hiyerarşisi ve Adres Haritası

### 5.1 Versal Adres Haritası

Versal, 44-bit fiziksel adres alanı kullanır (16 TB):

```
    0x0000_0000_0000 ┌─────────────────────────┐
                     │    DDR Low (2GB)        │  ◄── LPDDR4
    0x0000_8000_0000 ├─────────────────────────┤
                     │    Reserved             │
    0x0000_A400_0000 ├─────────────────────────┤
                     │    PL Address Space     │  ◄── DMA Control Registers
    0x0000_A500_0000 ├─────────────────────────┤
                     │    ...                  │
    0x0000_FFFC_0000 ├─────────────────────────┤
                     │    OCM (256KB)          │  ◄── On-Chip Memory
    0x0001_0000_0000 ├─────────────────────────┤
                     │    ...                  │
    0x0008_0000_0000 ├─────────────────────────┤
                     │    DDR High (6GB+)      │  ◄── Extended DDR
    0x0010_0000_0000 └─────────────────────────┘
```

### 5.2 Bellek Türlerinin Karakteristikleri

#### 5.2.1 LPDDR4

| Parametre | Değer |
|-----------|-------|
| **Kapasite** | 2 GB |
| **Başlangıç Adresi** | 0x0000_0000 |
| **Bitiş Adresi** | 0x7FFF_FFFF |
| **Bant Genişliği** | ~17 GB/s (teorik) |
| **Burst Length** | 16 (fixed) |
| **Erişim Gecikmesi** | ~80-100 ns |

#### 5.2.2 OCM (On-Chip Memory)

| Parametre | Değer |
|-----------|-------|
| **Kapasite** | 256 KB |
| **Başlangıç Adresi** | 0xFFFC_0000 |
| **Bitiş Adresi** | 0xFFFF_FFFF |
| **Erişim Gecikmesi** | ~10-20 ns |
| **Özellik** | Cache bypass, düşük gecikme |

### 5.3 Cache Coherency Konuları

Versal'da DMA transferlerinde cache coherency kritik bir konudur:

#### 5.3.1 Coherent Transfer (HPC Port)
- Cache ile otomatik senkronizasyon
- Ek CPU müdahalesi gerekmez
- Daha yüksek gecikme

#### 5.3.2 Non-Coherent Transfer (HP Port)
- Manuel cache yönetimi gerekli
- Daha düşük gecikme
- `Xil_DCacheFlushRange()` / `Xil_DCacheInvalidateRange()` kullanımı

---

## 6. Network-on-Chip (NoC) Mimarisi

### 6.1 NoC Kavramı

Versal'ın en yenilikçi özelliklerinden biri, on-chip interconnect için kullanılan Network-on-Chip (NoC) mimarisidir. Geleneksel bus tabanlı interconnect'lerin aksine, NoC paket-tabanlı bir ağ yapısı kullanır.

### 6.2 NoC Topolojisi

```
    ┌─────────────────────────────────────────────────────────┐
    │                     NoC FABRIC                          │
    │                                                         │
    │   NMU ──────► NSU ──────► NMU ──────► NSU              │
    │    │          │           │          │                 │
    │    ▼          ▼           ▼          ▼                 │
    │   ═══════════════════════════════════════              │
    │         NoC Packet Switching Network                    │
    │   ═══════════════════════════════════════              │
    │    │          │           │          │                 │
    │    ▼          ▼           ▼          ▼                 │
    │   NPS        NPS         NPS        NPS                │
    │    │          │           │          │                 │
    │    ▼          ▼           ▼          ▼                 │
    │   MC0        MC1         DDR        PL                 │
    └─────────────────────────────────────────────────────────┘

    NMU: NoC Master Unit (Initiator)
    NSU: NoC Slave Unit (Target)
    NPS: NoC Packet Switch
    MC: Memory Controller
```

### 6.3 NoC Bileşenleri

#### 6.3.1 NoC Master Unit (NMU)
- AXI master'ları NoC'a bağlar
- AXI → NoC protokol dönüşümü
- QoS parametreleri uygular

#### 6.3.2 NoC Slave Unit (NSU)
- NoC'u AXI slave'lere bağlar
- NoC → AXI protokol dönüşümü
- Address decoding

#### 6.3.3 NoC Packet Switch (NPS)
- Paket yönlendirme
- Arbitration
- Flow control

### 6.4 NoC QoS (Quality of Service)

NoC, farklı trafik türleri için QoS desteği sağlar:

| QoS Sınıfı | Açıklama | Kullanım |
|------------|----------|----------|
| **Best Effort** | Garanti yok | Genel amaçlı |
| **Low Latency** | Düşük gecikme öncelikli | Real-time kontrol |
| **Isochronous** | Sabit bant genişliği | Video streaming |

### 6.5 NoC Bant Genişliği Yönetimi

Bu projede karşılaşılan önemli bir kısıtlama, NoC bant genişliği bütçelemesidir:

```
Toplam NoC BW Limiti: ~16 GB/s (MC başına)

Örnek Dağılım:
├── PS FPD Traffic:     4 GB/s
├── PS LPD Traffic:     2 GB/s
├── PL DMA Traffic:     8 GB/s
│   ├── AXI DMA:        3 GB/s
│   ├── AXI CDMA:       3 GB/s
│   └── AXI MCDMA:      2 GB/s
└── Diğer:              2 GB/s
```

### 6.6 LPD_AXI_NOC Bağlantısı

Bu projenin kritik teknik çözümlerinden biri, LPD DMA'nın NoC üzerinden DDR'a erişimini sağlamaktır:

#### 6.6.1 Problem
Varsayılan konfigürasyonda LPD DMA yalnızca OCM'e erişebilir. DDR erişimi için özel NoC path gereklidir.

#### 6.6.2 Çözüm
`PS_USE_NOC_LPD_AXI0 = 1` parametresi ile:
- CIPS'te LPD_AXI_NOC_0 interface'i aktifleşir
- Board automation bu interface'i NoC'a bağlar
- LPD DMA → LPD_AXI_NOC_0 → NoC → DDR MC yolu oluşur

---

## 7. Sistem Tasarım Mimarisi

### 7.1 Blok Tasarım Genel Görünümü

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           VERSAL DMA BENCHMARK DESIGN                        │
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                              CIPS                                    │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │   │
│  │  │    APU      │  │    LPD      │  │    PMC      │                  │   │
│  │  │  (A72x2)    │  │  (R5Fx2)    │  │             │                  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └─────────────┘                  │   │
│  │         │                │                                           │   │
│  │         │         ┌──────┴──────┐                                   │   │
│  │         │         │   LPD DMA   │ ◄── 8 Channel ADMA                │   │
│  │         │         │   (ADMA)    │                                   │   │
│  │         │         └──────┬──────┘                                   │   │
│  │         │                │                                           │   │
│  │    M_AXI_FPD       LPD_AXI_NOC_0                                    │   │
│  │         │                │                                           │   │
│  └─────────┼────────────────┼──────────────────────────────────────────┘   │
│            │                │                                              │
│            ▼                ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                           AXI NoC                                    │   │
│  │                                                                      │   │
│  │   S00: FPD_CCI_0    S04: LPD_AXI_NOC    S06: CDMA                   │   │
│  │   S01: FPD_CCI_1    S05: PMC_NOC        S07: DMA                    │   │
│  │   S02: FPD_CCI_2                        S08: MCDMA                  │   │
│  │   S03: FPD_CCI_3                                                    │   │
│  │                                                                      │   │
│  │                    ┌────────────────┐                               │   │
│  │                    │    MC_0        │ ──────► LPDDR4 (2GB)          │   │
│  │                    │  (LPDDR4 MC)   │                               │   │
│  │                    └────────────────┘                               │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                        PL DOMAIN                                    │    │
│  │                                                                     │    │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐           │    │
│  │  │   AXI CDMA    │  │   AXI DMA     │  │  AXI MCDMA    │           │    │
│  │  │               │  │               │  │               │           │    │
│  │  │  MM-to-MM     │  │  MM2S ◄─► S2MM│  │  16 CH MM2S   │           │    │
│  │  │  SG Mode      │  │  26-bit BTT   │  │  16 CH S2MM   │           │    │
│  │  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘           │    │
│  │          │                  │                  │                    │    │
│  │          │           ┌──────┴──────┐          │                    │    │
│  │          │           │ AXIS FIFO   │          │                    │    │
│  │          │           │ (Loopback)  │          │                    │    │
│  │          │           └─────────────┘   ┌──────┴──────┐             │    │
│  │          │                             │ AXIS FIFO   │             │    │
│  │          │                             │ (Loopback)  │             │    │
│  │          │                             └─────────────┘             │    │
│  │          │                                                         │    │
│  │     SmartConnect                   SmartConnect                    │    │
│  │          │                              │                          │    │
│  │          └──────────────┬───────────────┘                          │    │
│  │                         │                                          │    │
│  │                    To AXI NoC                                      │    │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Interrupt Yapısı

```
┌────────────────────────────────────────────────────┐
│                 INTERRUPT ROUTING                   │
│                                                     │
│   AXI CDMA ──────► IRQ[0] ─┐                       │
│   AXI DMA  ──────► IRQ[1] ─┼──► xlconcat ──► CIPS │
│   AXI MCDMA ─────► IRQ[2] ─┤      (4-bit)    F2P  │
│   Reserved ──────► IRQ[3] ─┘                       │
│                                                     │
│   LPD DMA: Internal PS interrupt (IRQ #90-97)      │
└────────────────────────────────────────────────────┘
```

### 7.3 Clock Dağıtımı

| Clock | Kaynak | Frekans | Kullanım |
|-------|--------|---------|----------|
| **pl0_ref_clk** | CIPS | 100 MHz | PL logic, DMA'lar |
| **noc_clk** | NoC Internal | 1 GHz | NoC fabric |
| **ddr_clk** | MC Internal | ~533 MHz | LPDDR4 interface |

---

## 8. LPD DMA ve DDR Erişim Problemi

### 8.1 Problemin Tanımı

Versal mimarisinde LPD DMA (ADMA), varsayılan olarak yalnızca Low Power Domain içindeki kaynaklara erişebilir:
- OCM (On-Chip Memory)
- LPD peripherals
- TCM (RPU Tightly Coupled Memory)

Ancak birçok uygulama için LPD DMA'nın DDR belleğe erişmesi gerekir:
- Büyük buffer'lar için yeterli OCM kapasitesi yok (256 KB)
- RPU tabanlı uygulamalarda DDR kullanımı yaygın
- Düşük güç modunda DDR erişimi gerekebilir

### 8.2 Mimari Kısıtlama

```
    YANLIŞ ANLAYIŞ:
    ═══════════════
    LPD DMA ──X──► DDR (Direkt erişim YOK)

    DOĞRU MİMARİ:
    ═════════════
    LPD DMA ─────► LPD_AXI_NOC_0 ─────► NoC ─────► DDR MC ─────► DDR
                        │
                        └── Bu interface'in aktifleştirilmesi gerekir
```

### 8.3 Çözüm: PS_USE_NOC_LPD_AXI0

#### 8.3.1 CIPS Konfigürasyonu

CIPS IP'sinde aşağıdaki parametre aktifleştirilmelidir:

```
PS_PMC_CONFIG {
    ...
    PS_USE_NOC_LPD_AXI0 {1}
    ...
}
```

#### 8.3.2 Board Automation Etkisi

Bu parametre aktif olduğunda:
1. CIPS üzerinde `LPD_AXI_NOC_0` interface'i görünür hale gelir
2. VPK120 board automation bu interface'i otomatik olarak NoC'a bağlar
3. NoC, DDR Memory Controller'a path oluşturur

#### 8.3.3 Sonuç Adres Erişimi

| Kaynak | Erişilebilir Adres Aralığı |
|--------|---------------------------|
| DDR | 0x0000_0000 - 0x7FFF_FFFF (2 GB) |
| OCM | 0xFFFC_0000 - 0xFFFF_FFFF (256 KB) |

### 8.4 Alternatif Çözümler (Değerlendirildi, Seçilmedi)

| Alternatif | Dezavantaj |
|------------|------------|
| M_AXI_LPD kullanımı | Adres aperture uyumsuzluğu |
| AXI Bridge ekleme | Ek gecikme, kaynak kullanımı |
| Yalnızca OCM kullanımı | 256 KB kapasite limiti |

---

## 9. Benchmark Metodolojisi

### 9.1 Test Kategorileri

#### 9.1.1 Throughput Testleri

Maksimum veri transfer hızını ölçmek için:

| Test | Açıklama |
|------|----------|
| **Unidirectional** | Tek yönlü transfer (Read veya Write) |
| **Bidirectional** | Eşzamanlı Read + Write |
| **Burst Size Sweep** | Farklı burst boyutlarında performans |

#### 9.1.2 Latency Testleri

Transfer başlatma ve tamamlanma arasındaki süre:

| Test | Açıklama |
|------|----------|
| **First Byte Latency** | İlk veri byte'ının ulaşma süresi |
| **Completion Latency** | Toplam transfer tamamlanma süresi |
| **Interrupt Latency** | Kesme yanıt süresi |

#### 9.1.3 Stress Testleri

Uzun süreli güvenilirlik:

| Test | Açıklama |
|------|----------|
| **Continuous Transfer** | Kesintisiz transfer (1+ saat) |
| **Error Rate** | Veri bütünlüğü hata oranı |
| **Temperature Impact** | Sıcaklık altında performans |

### 9.2 Transfer Boyutu Matrisi

```
    Küçük        Orta         Büyük        Çok Büyük
    ─────        ────         ─────        ─────────
    64 B         1 KB         64 KB        1 MB
    128 B        2 KB         128 KB       4 MB
    256 B        4 KB         256 KB       16 MB
    512 B        8 KB         512 KB       64 MB
                 16 KB
                 32 KB
```

### 9.3 Veri Pattern'leri

| Pattern | Değer | Amaç |
|---------|-------|------|
| **Incremental** | 0x00, 0x01, 0x02... | Temel bütünlük kontrolü |
| **All-Ones** | 0xFF, 0xFF... | Maksimum toggle testi |
| **All-Zeros** | 0x00, 0x00... | Minimum toggle testi |
| **Checkerboard** | 0xAA, 0x55... | Bit-level integrity |
| **Random** | PRNG generated | Gerçek dünya simülasyonu |

### 9.4 Memory-to-Memory Test Matrisi

CDMA için tüm bellek kombinasyonları test edilir:

```
              │ DDR  │ OCM  │
    ──────────┼──────┼──────┤
    DDR       │  ✓   │  ✓   │
    OCM       │  ✓   │  ✓   │
```

### 9.5 Ölçüm Yöntemi

#### 9.5.1 Zaman Ölçümü

```
Performance Timer:
├── ARM Generic Timer (CNT_CT)
├── Çözünürlük: ~1 ns
└── Overflow: ~584 yıl @ 1 GHz
```

#### 9.5.2 Throughput Hesaplama

```
Throughput (MB/s) = Transfer Boyutu (Bytes) / Transfer Süresi (μs)
```

#### 9.5.3 Verimlilik Hesaplama

```
Verimlilik (%) = Ölçülen Throughput / Teorik Maksimum × 100
```

---

## 10. Performans Metrikleri ve Beklentiler

### 10.1 Teorik Maksimum Değerler

| DMA | Interface | Frekans | Teorik Max |
|-----|-----------|---------|------------|
| LPD DMA | 128-bit AXI | 600 MHz | 9.6 GB/s |
| AXI DMA | 128-bit AXI | 100 MHz | 1.6 GB/s |
| AXI CDMA | 128-bit AXI | 100 MHz | 1.6 GB/s |
| AXI MCDMA | 128-bit AXI | 100 MHz | 1.6 GB/s |

### 10.2 Gerçekçi Beklentiler

Overhead'ler nedeniyle gerçek performans teorik değerin %60-80'i civarındadır:

| DMA | Beklenen Throughput | Beklenen Latency |
|-----|---------------------|------------------|
| LPD DMA | 5-7 GB/s | 0.2-0.5 μs |
| AXI DMA | 1.0-1.3 GB/s | 0.5-1.0 μs |
| AXI CDMA | 1.2-1.5 GB/s | 0.3-0.7 μs |
| AXI MCDMA | 0.8-1.2 GB/s (per ch) | 0.5-1.0 μs |

### 10.3 Performansı Etkileyen Faktörler

#### 10.3.1 Olumlu Faktörler
- Aligned addresses (64-byte boundary)
- Büyük transfer boyutları
- Scatter-Gather mode (birden fazla transfer için)
- Cache-coherent access (HPC port)

#### 10.3.2 Olumsuz Faktörler
- Unaligned addresses
- Küçük transfer boyutları (overhead dominant)
- Descriptor fetch overhead
- NoC congestion
- DDR bank conflicts

### 10.4 Karşılaştırmalı Analiz

```
    USE CASE RECOMMENDATION MATRIX
    ══════════════════════════════

    Senaryo                      │ Önerilen DMA    │ Sebep
    ─────────────────────────────┼─────────────────┼──────────────────
    Büyük bellek kopyalama       │ AXI CDMA        │ En yüksek MM throughput
    Stream peripheral I/O        │ AXI DMA         │ Stream interface
    Multi-channel streaming      │ AXI MCDMA       │ Çoklu kanal desteği
    Düşük güç uygulaması        │ LPD DMA         │ LPD domain avantajı
    Real-time RPU uygulaması    │ LPD DMA         │ RPU domain bağlantısı
    En düşük gecikme            │ LPD DMA / CDMA  │ Hard IP / No stream OH
```

---

## 11. Tasarım Kararları ve Trade-off'lar

### 11.1 DMA Veri Genişliği Seçimi

**Karar:** 128-bit veri genişliği

| Seçenek | Avantaj | Dezavantaj |
|---------|---------|------------|
| 64-bit | Düşük kaynak | Düşük throughput |
| **128-bit** | **Dengeli** | **Makul kaynak** |
| 256-bit | Yüksek throughput | Yüksek kaynak |
| 512-bit | Maksimum throughput | Çok yüksek kaynak |

### 11.2 Scatter-Gather vs Simple Mode

**Karar:** Scatter-Gather mode aktif

| Mod | Kullanım Senaryosu |
|-----|-------------------|
| Simple | Tek, küçük transfer |
| **SG** | **Birden fazla transfer, büyük veri** |

### 11.3 Buffer Length Width (AXI DMA)

**Karar:** 26-bit (64 MB maksimum)

| Genişlik | Max Transfer | Seçim Sebebi |
|----------|--------------|--------------|
| 14-bit | 16 KB | Varsayılan, yetersiz |
| 23-bit | 8 MB | Orta, çoğu uygulama için yeterli |
| **26-bit** | **64 MB** | **Büyük buffer testleri için** |

### 11.4 Loopback FIFO Kullanımı

**Karar:** AXI DMA ve MCDMA için loopback FIFO

**Sebep:** Stream DMA'ları test etmek için harici peripheral gerekmeden self-test imkanı sağlar.

```
    MM2S ─────► FIFO ─────► S2MM
         Stream     Stream
```

### 11.5 NoC Bandwidth Allocation

**Karar:** Her DMA için 1 GB/s tahsis

| DMA | Read BW | Write BW | Toplam |
|-----|---------|----------|--------|
| CDMA | 1 GB/s | 1 GB/s | 2 GB/s |
| DMA | 1 GB/s | 1 GB/s | 2 GB/s |
| MCDMA | 1 GB/s | 1 GB/s | 2 GB/s |
| **Toplam** | | | **6 GB/s** |

Bu değerler, NoC'un 16 GB/s limitinin altında kalarak congestion'ı önler.

### 11.6 Clock Frekansı

**Karar:** 100 MHz PL clock

| Frekans | Avantaj | Dezavantaj |
|---------|---------|------------|
| 50 MHz | Kolay timing closure | Düşük performans |
| **100 MHz** | **Dengeli** | **Standart** |
| 200 MHz | Yüksek performans | Timing zorlukları |
| 300 MHz | Maksimum performans | Çok zor timing |

---

## 12. Sonuç ve Gelecek Çalışmalar

### 12.1 Proje Çıktıları

Bu proje kapsamında:

1. **Donanım Tasarımı:** Tüm DMA türlerini içeren Versal block design
2. **PDI/XSA:** Programlanabilir device image ve hardware specification
3. **Benchmark Yazılımı:** Kapsamlı DMA test uygulaması
4. **Dokümantasyon:** Mimari ve kullanım kılavuzları

### 12.2 Teknik Başarılar

- LPD DMA'nın DDR erişim probleminin NoC path ile çözülmesi
- 26-bit buffer length ile 64 MB'a kadar tek transfer desteği
- Multi-channel DMA ile paralel transfer kapasitesi
- Kapsamlı benchmark altyapısı

### 12.3 Gelecek Çalışmalar

| Alan | Potansiyel Geliştirme |
|------|----------------------|
| **QDMA** | PCIe üzerinden host-device DMA |
| **AI Engine** | AI Engine tile'ları ile DMA entegrasyonu |
| **Multi-DDR** | DDR4 SODIMM eklenmesi |
| **HBM** | HBM destekli cihazlarda test |
| **Power** | Güç tüketimi analizi |

### 12.4 Öğrenilen Dersler

1. **NoC Konfigürasyonu Kritik:** Versal'da veri hareketi NoC üzerinden geçer, doğru konfigürasyon şart
2. **Board Automation Kullanın:** Manuel bağlantı yerine board automation güvenilir sonuç verir
3. **Path Length Dikkat:** Windows'ta Vivado için path uzunluk limitlerine dikkat
4. **Parallel Synthesis:** Bellek kısıtlamalı sistemlerde parallel job sayısını azaltın

---

## Referanslar

1. AMD/Xilinx, "Versal ACAP Technical Reference Manual (AM011)"
2. AMD/Xilinx, "Versal ACAP Programmable Network on Chip and Integrated Memory Controller (PG313)"
3. AMD/Xilinx, "AXI DMA LogiCORE IP Product Guide (PG021)"
4. AMD/Xilinx, "AXI CDMA LogiCORE IP Product Guide (PG034)"
5. AMD/Xilinx, "AXI MCDMA LogiCORE IP Product Guide (PG288)"
6. AMD/Xilinx, "Zynq UltraScale+ MPSoC ZDMA Controller (Zynq UG1087)" - ADMA referansı için
7. AMD/Xilinx, "VPK120 Evaluation Board User Guide (UG1568)"

---

**Doküman Sonu**

*Bu doküman, Versal DMA Benchmark projesi kapsamında hazırlanmıştır.*
