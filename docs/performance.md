# Protobuf Performance
The following benchmark test results were produced on a workstation utilizing an Intel® Xeon® Processor E5-2630 with 32GB of RAM.

This table contains the results of three separate languages:

* **C++** - For C++, there are three parsing methods:
	* **new** - This is for using a new operator for creating a message instance.
    * **new arena** - This is for using arena for creating a new message instance.
    * **reuse** - This is for reusing the same message instance for parsing.
* **Java** - For Java, there are three parsing/serialization methods:
	* **byte[]** - This is for parsing from a Byte Array.
    * **ByteString** - This is for parsing from a
    	com.google.protobuf.ByteString.
    * **InputStream** - This is for parsing from an InputStream.
* **Python** - For Python, there are three types of Python protobuf for testing:
	* **C++-generated-code** - This is for using C++ generated code of the
    	proto file as a dynamic linked library.
	* **C++-reflection** - This is for using C++ reflection, for which there's no
    	generated code, but still using C++ protobuf library as a dynamic linked
        library.
	* **pure-Python** - This is for the pure version of Python, which does not link with
    	any C++ protobuf library.

## Parsing performance

<table>
<tbody><tr>
<th rowspan="2"> </th>
<th colspan="3" rowspan="1">C++</th>
<th colspan="3" rowspan="1">C++ with tcmalloc</th>
<th colspan="3" rowspan="1">java</th>
<th colspan="3" rowspan="1">python</th>
</tr>
<tr>
<th colspan="1">new</th>
<th colspan="1">new arena</th>
<th colspan="1">reuse</th>
<th colspan="1">new</th>
<th colspan="1">new arena</th>
<th colspan="1">reuse</th>
<th colspan="1">byte[]</th>
<th colspan="1">ByteString</th>
<th colspan="1">InputStream</th>
<th colspan="1">C++-generated-code</th>
<th colspan="1">C++-reflection</th>
<th colspan="1">pure-Python</th>
</tr>
<tr>
<td>google_message1_proto2</td>
<td>368.717MB/s</td>
<td>261.847MB/s</td>
<td>799.403MB/s</td>
<td>645.183MB/s</td>
<td>441.023MB/s</td>
<td>1.122GB/s</td>
<td>425.437MB/s</td>
<td>425.937MB/s</td>
<td>251.018MB/s</td>
<td>82.8314MB/s</td>
<td>47.6763MB/s</td>
<td>3.76299MB/s</td>
</tr>
<tr>
<td>google_message1_proto3</td>
<td>294.517MB/s</td>
<td>229.116MB/s</td>
<td>469.982MB/s</td>
<td>434.510MB/s</td>
<td>394.701MB/s</td>
<td>591.931MB/s</td>
<td>357.597MB/s</td>
<td>378.568MB/s</td>
<td>221.676MB/s</td>
<td>82.0498MB/s</td>
<td>39.9467MB/s</td>
<td>3.77751MB/s</td>
</tr>
<tr>
<td>google_message2</td>
<td>277.242MB/s</td>
<td>347.611MB/s</td>
<td>793.67MB/s</td>
<td>503.721MB/s</td>
<td>596.333MB/s</td>
<td>922.533MB/s</td>
<td>416.778MB/s</td>
<td>419.543MB/s</td>
<td>367.145MB/s</td>
<td>241.46MB/s</td>
<td>71.5723MB/s</td>
<td>2.73538MB/s</td>
</tr>
<tr>
<td>google_message3_1</td>
<td>213.478MB/s</td>
<td>291.58MB/s</td>
<td>543.398MB/s</td>
<td>539.704MB/s</td>
<td>717.300MB/s</td>
<td>927.333MB/s</td>
<td>684.241MB/s</td>
<td>704.47MB/s</td>
<td>648.624MB/s</td>
<td>209.036MB/s</td>
<td>142.356MB/s</td>
<td>15.3324MB/s</td>
</tr>
<tr>
<td>google_message3_2</td>
<td>672.685MB/s</td>
<td>802.767MB/s</td>
<td>1.21505GB/s</td>
<td>985.790MB/s</td>
<td>1.136GB/s</td>
<td>1.367GB/s</td>
<td>1.54439GB/s</td>
<td>1.60603GB/s</td>
<td>1.33443GB/s</td>
<td>573.835MB/s</td>
<td>314.33MB/s</td>
<td>15.0169MB/s</td>
</tr>
<tr>
<td>google_message3_3</td>
<td>207.681MB/s</td>
<td>140.591MB/s</td>
<td>535.181MB/s</td>
<td>369.743MB/s</td>
<td>262.301MB/s</td>
<td>556.644MB/s</td>
<td>279.385MB/s</td>
<td>304.853MB/s</td>
<td>107.575MB/s</td>
<td>32.248MB/s</td>
<td>26.1431MB/s</td>
<td>2.63541MB/s</td>
</tr>
<tr>
<td>google_message3_4</td>
<td>7.96091GB/s</td>
<td>7.10024GB/s</td>
<td>9.3013GB/s</td>
<td>8.518GB/s</td>
<td>8.171GB/s</td>
<td>9.917GB/s</td>
<td>5.78006GB/s</td>
<td>5.85198GB/s</td>
<td>4.62609GB/s</td>
<td>2.49631GB/s</td>
<td>2.35442GB/s</td>
<td>802.061MB/s</td>
</tr>
<tr>
<td>google_message3_5</td>
<td>76.0072MB/s</td>
<td>51.6769MB/s</td>
<td>237.856MB/s</td>
<td>178.495MB/s</td>
<td>111.751MB/s</td>
<td>329.569MB/s</td>
<td>121.038MB/s</td>
<td>132.866MB/s</td>
<td>36.9197MB/s</td>
<td>10.3962MB/s</td>
<td>8.84659MB/s</td>
<td>1.25203MB/s</td>
</tr>
<tr>
<td>google_message4</td>
<td>331.46MB/s</td>
<td>404.862MB/s</td>
<td>427.99MB/s</td>
<td>589.887MB/s</td>
<td>720.367MB/s</td>
<td>705.373MB/s</td>
<td>606.228MB/s</td>
<td>589.13MB/s</td>
<td>530.692MB/s</td>
<td>305.543MB/s</td>
<td>174.834MB/s</td>
<td>7.86485MB/s</td>
</tr>
</tbody></table>

## Serialization performance

<table>
<tbody><tr>
<th rowspan="2"> </th>
<th colspan="1" rowspan="2">C++</th>
<th colspan="1" rowspan="2">C++ with tcmalloc</th>
<th colspan="3" rowspan="1">java</th>
<th colspan="3" rowspan="1">python</th>
</tr>
<tr>
<th colspan="1">byte[]</th>
<th colspan="1">ByteString</th>
<th colspan="1">InputStream</th>
<th colspan="1">C++-generated-code</th>
<th colspan="1">C++-reflection</th>
<th colspan="1">pure-Python</th>
</tr>
<tr>
<td>google_message1_proto2</td>
<td>1.39698GB/s</td>
<td>1.701GB/s</td>
<td>1.12915GB/s</td>
<td>1.13589GB/s</td>
<td>758.609MB/s</td>
<td>260.911MB/s</td>
<td>58.4815MB/s</td>
<td>5.77824MB/s</td>
</tr>
<tr>
<td>google_message1_proto3</td>
<td>959.305MB/s</td>
<td>939.404MB/s</td>
<td>1.15372GB/s</td>
<td>1.07824GB/s</td>
<td>802.337MB/s</td>
<td>239.4MB/s</td>
<td>33.6336MB/s</td>
<td>5.80524MB/s</td>
</tr>
<tr>
<td>google_message2</td>
<td>1.27429GB/s</td>
<td>1.402GB/s</td>
<td>1.01039GB/s</td>
<td>1022.99MB/s</td>
<td>798.736MB/s</td>
<td>996.755MB/s</td>
<td>57.9601MB/s</td>
<td>4.09246MB/s</td>
</tr>
<tr>
<td>google_message3_1</td>
<td>1.31916GB/s</td>
<td>2.049GB/s</td>
<td>991.496MB/s</td>
<td>860.332MB/s</td>
<td>662.88MB/s</td>
<td>1.48625GB/s</td>
<td>421.287MB/s</td>
<td>18.002MB/s</td>
</tr>
<tr>
<td>google_message3_2</td>
<td>2.15676GB/s</td>
<td>2.632GB/s</td>
<td>2.14736GB/s</td>
<td>2.08136GB/s</td>
<td>1.55997GB/s</td>
<td>2.39597GB/s</td>
<td>326.777MB/s</td>
<td>16.0527MB/s</td>
</tr>
<tr>
<td>google_message3_3</td>
<td>650.456MB/s</td>
<td>1.040GB/s</td>
<td>593.52MB/s</td>
<td>580.667MB/s</td>
<td>346.839MB/s</td>
<td>123.978MB/s</td>
<td>35.893MB/s</td>
<td>2.32834MB/s</td>
</tr>
<tr>
<td>google_message3_4</td>
<td>8.70154GB/s</td>
<td>9.825GB/s</td>
<td>5.88645GB/s</td>
<td>5.93946GB/s</td>
<td>2.44388GB/s</td>
<td>5.9241GB/s</td>
<td>4.05837GB/s</td>
<td>876.87MB/s</td>
</tr>
<tr>
<td>google_message3_5</td>
<td>246.33MB/s</td>
<td>443.993MB/s</td>
<td>283.278MB/s</td>
<td>259.167MB/s</td>
<td>206.37MB/s</td>
<td>37.0285MB/s</td>
<td>12.2228MB/s</td>
<td>1.1979MB/s</td>
</tr>
<tr>
<td>google_message4</td>
<td>1.56674GB/s</td>
<td>2.19601GB/s</td>
<td>776.907MB/s</td>
<td>770.707MB/s</td>
<td>702.931MB/s</td>
<td>1.49623GB/s</td>
<td>205.116MB/s</td>
<td>8.93428MB/s</td>
</tr>
</tbody></table>

\* The cpp performance can be improved by using [tcmalloc](https://gperftools.github.io/gperftools/tcmalloc.html), please follow the (instruction)[https://github.com/protocolbuffers/protobuf/blob/master/benchmarks/README.md] to link with tcmalloc to get the faster result.
