********************************************

           KM-Z80 midi ver 0.1 alpha

    Copyright (C) 2019 Katsumi Morimatsu

       kmorimatsu@users.sourceforge.jp
        https://github.com/kmorimatsu
  http://hp.vector.co.jp/authors/VA016157/

********************************************

このソフトウェアは、非商用でMicrochip社の半導体に使われる場合に限り、フリー
のライセンスの元、使用可能です。これは、ソースコードのファイルのうち幾つかは、
LGPL ver 2.1のライセンスで供給されているものの、一部のファイルがMachiKania
ライセンス、Microchip社のライセンス、及び非商用フリーのライセンスで供給されて
いる事に依ります。個々のソースファイルのライセンスについては、ファイルに記載
されているコメントを参照して下さい。

The license is written in Japanese. The following is a translatin, but only
the original Japanese license above is valid.

This software is provided under the lisence that is free only when it is used
for non-profit purpose and for Microchip products. Many files of source codes
are provided under LGPL v2.1 license, but some ones are provided under the 
MachiKania license, Microchip license and non-porofit free license. Please
see the comments in source codes for each file's licenses.

＜概要＞
KM-Z80 midiは、Sharp MZ-80K パーソナルコンピューターの互換機です。プログラム
のロードとセーブは、USB メモリーを介して行なう事が出来ます。また、キーボード
も内蔵していますので、別途用意する必要はありません。+5Vの電源とUSBメモリー、
それにNTSCのビデオ表示器を接続する事で動きます。また、電源投下時にすでにBASIC
がロードされているので、すぐにそれを実行する事が可能です。

＜序論＞
MZ-80Kは、SHARPが1978年に発売した、8bitパーソナルコンピューターです。発
売当時、ほぼ同時期に発売された日立のベーシックマスターや、少し遅れて販売
されたNECのPC-8001と並んで、高級言語であるBASICを個人のレベルで使えるマイ
クロコンピューターとして、一世風靡しました。

MZ-80Kには、マウスなどのポインティングデバイスはなかったものの、文字や
グラフィックを表示・入力するためのディスプレイやキーボードを備え、当時高
性能のCPUであったZ80が、これまた当時は大容量であった20 KBのRAMで動きまし
た。

外部記憶装置としてカセットテープレコーダーが付属していました。読み書きの
速度は平均で1400ボーであり、これは当時の主流であったカンザスシティスタン
ダードの300ボートと比べて、夢のような速度でした。また、レコーダーが本体に
付属しているために読み書きの安定性も抜群でした。

このように、MZ-80Kは、今日のパーソナルコンピューターが持つ基本特性を、小
規模であるものの、ほとんど備えており、日本のパーソナルコンピューターのさ
きがけとも言える存在です。他方で、その構造はいたってシンプルであり、その
全回路図が公開されていました。MZ-80Kのこういった特性は、コンピューターの
動作原理を理解するうえで、貴重なものです。パーソナルコンピューターはその
後、発展に発展を遂げ、非常に複雑になりました。現在では、個人のレベルでこ
ういったコンピューターの全体像を把握することは、プロの方々でも難しいと言
えるでしょう。

今日、MZ-80K本体を購入することはかなり難しくなっています（まれにオーク
ションに出品されることがあります）。しかし、そのアッパーコンパチブルであ
るMZ-700のエミュレーター(MZ-700 Emulator MZ700WIN For Windows)が公開され
ているので、現在でもそれがどんなものであったのかを伺い知ることが出来ます。
また、拙作のMZ-80Kレプリカ（KM-Z80; 回路図及びプログラムを公開済み）を作
成すれば、MZ-80Kの動作環境を再現することが出来ます。

こういったエミュレーターやレプリカなどを利用する上でしばしば問題になる
のが、その上で動作するプログラムです。ROM上にある基本プログラム（ここで
は、モニタープログラムと呼ぶことにします）は、コンピューターの動作に必須
ですが、それを販売した会社（MZ-80Kの場合は、SHARP）が著作権を有するため、
オリジナルのコンピューターを有しているのでない限り、使用することが出来ま
せん。Apple Iのレプリカの場合、モニタープログラムを作成した技術者から許
可を得ることで、この問題をクリアーしています。MZ80Kの場合、オリジナルの
モニタープログラム(SP-1002)とは著作権の異なるモニタープログラムが公表さ
れており(MZ-NEW MONITER)、それを用いることでこの問題をクリアーしていま
す。

MZ-80Kのようなレトロコンピューターを考える上で、高級言語であるBASICは
切っても切り離せない存在です。MZ-80K用には、SHARPオリジナルのSP-5010や
SP-5030、ハドソンソフトのHu-BASICなどがあります。いずれもカセットテープ
に入った状態で販売されていましたが、今日、こういったソフトウェアを入手
することはほとんど不可能です。

そこで、MZ-700エミュレーターやMZ-80Kレプリカ上で使用可能なBASICを、独
自に作成してあります。KM-BASIC for MZ-80K/MZ-700がそれです。KM-Z80では、
電源投下時に、すでにKM-BASICがロードされた状態になっており、「GOTO$1200」
を実行するだけですぐにBASICによるプログラミングが出来るようになっている
のが、特徴です。

今までに、MZ-80K互換機として、幾つかのシリーズのKM-Z80を公開してきました。
Zilog社の40ピンのLSIを用いたKM-Z80、Microship社のPIC32MX150F128Bを用いた
KM-Z80 mini、ブラウザーでHTML5アプリケーションとして走るKM-Z80 web、
PIC32MX350F256Bを用いたKM-Z80C mini、MachiKania用のアプリケーションであ
る、KM-Z80sdなどです。KM-Z80 webを除く他の実装では、キーボードとして
PS/2キーボードが必要で、また、キー配列がオリジナルのMZ-80Kと異なるために
色々とキー入力の不都合がありました。そこで、KM-Z80 midiでは、オリジナルと
全く同じ配列のキーボードを内蔵する事にしました。また、外部記憶機器として
USBメモリーが使えるようにした事も特徴の一つです。インターフェース部の回路
を工夫する事で、DIP-28のPIC32MX270F256Bに１チップで収める事が出来ました。
また、MZ-80Kの最大RAM容量である48K bytesでの実行も実現しています。１チップ
でこれらの機能を全てこなせるように設計した、こだわりの作品です。

＜使用方法＞
回路図を添付してありますので、それを参考に製作して下さい。キーボード部に
使われている10k Ωの抵抗は、誤差が1%以下のものを使う事をお勧めします。

電源を入れると、MZ-NEW MONITORが起動します。使い方はSP-1002とほとんど変わ
りません。この状態で「GOTO$1200」と入力すると、すぐにKM-BASICを使用可能で
す。

USB メモリーは、電源投下後も抜き差しする事が可能です。ただし、ファイルの
ロード中やセーブ中に抜くと、ファイル構造が破壊される可能性があるので、注
意して下さい。なお、ディレクトリーリストが表示された後に抜く事は可能です。

USB メモリーを指さない状態、あるいはフォーマットが不正なUSB メモリーを指
した状態などでプログラムをロードしようとすると、「↓PLAY」の表示がでて
ストップします。この場合は、「Shift + Break」キーを押して、やり直して下
さい。

プログラムのロードもしくはセーブを実行すると、USB メモリーのルートディレ
クトリーにある、「*.MZT」ファイルのリストが表示されます。カーソルキーを
移動させて、「Space」キーを押す事で、ファイルを選択出来ます。なお、セー
ブの際に新規のファイル名を指定出来ませんので、予め上書き用のMZTファイルを
準備しておいて下さい。「*.MZF」や「*.M12」ファイルなども、拡張子をMZTに
書き換えれば使う事が出来ます。

その他の部分については、オリジナルのMZ-80Kとほぼ同様に扱う事が出来ます。
なお、「Shift」キーが二つありますが、左の「Shift」キーは、「１」「Ｑ」
「Ａ」「Ｚ」に対しては使えませんのでこの場合は右の「Shift」キーを使用し
て下さい。同様に「ー」「＝」等に関しては、左「Shift」キーを用いて下さい。

＜リリースノート＞

・ver 0.1 最初の公開αバージョン。
