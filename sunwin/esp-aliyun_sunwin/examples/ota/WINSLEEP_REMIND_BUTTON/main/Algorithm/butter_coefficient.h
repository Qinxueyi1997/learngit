#ifndef BUTTER_COEFFICIENT_H_INCLUDED
#define BUTTER_COEFFICIENT_H_INCLUDED

extern double filt_b[17][19] =
{
{0.0000000040054817368394007853, 0, -0.00000003604933563155460624, 0, 0.00000014419734252621842496, 0, -0.00000033646046589450964942, 0, 0.00000050469069884176455354, 0, -0.00000050469069884176455354, 0, 0.00000033646046589450964942, 0, -0.00000014419734252621842496, 0, 0.00000003604933563155460624, 0, -0.0000000040054817368394007853},
{0.0000000039549953465471552429, 0, -0.000000035594958118924400495, 0, 0.00000014237983247569760198, 0, -0.00000033221960910996102717, 0, 0.00000049832941366494154076, 0, -0.00000049832941366494154076, 0, 0.00000033221960910996102717, 0, -0.00000014237983247569760198, 0, 0.000000035594958118924400495, 0, -0.0000000039549953465471552429},
{0.0000000039635196147780910371, 0, -0.000000035671676533002820161, 0, 0.00000014268670613201128064, 0, -0.00000033293564764135969013, 0, 0.00000049940347146203956166, 0, -0.00000049940347146203956166, 0, 0.00000033293564764135969013, 0, -0.00000014268670613201128064, 0, 0.000000035671676533002820161, 0, -0.0000000039635196147780910371},
{0.0000000039645106236405272004, 0, -0.000000035680595612764743976, 0, 0.0000001427223824510589759, 0, -0.00000033301889238580424182, 0, 0.00000049952833857870638919, 0, -0.00000049952833857870638919, 0, 0.00000033301889238580424182, 0, -0.0000001427223824510589759, 0, 0.000000035680595612764743976, 0, -0.0000000039645106236405272004},
{0.0000000039645493884898567803, 0, -0.000000035680944496408710195, 0, 0.00000014272377798563484078, 0, -0.00000033302214863314797947, 0, 0.00000049953322294972188979, 0, -0.00000049953322294972188979, 0, 0.00000033302214863314797947, 0, -0.00000014272377798563484078, 0, 0.000000035680944496408710195, 0, -0.0000000039645493884898567803},
{0.0000000039644384790685998654, 0, -0.00000003567994631161740127, 0, 0.00000014271978524646960508, 0, -0.00000033301283224176241185, 0, 0.00000049951924836264359131, 0, -0.00000049951924836264359131, 0, 0.00000033301283224176241185, 0, -0.00000014271978524646960508, 0, 0.00000003567994631161740127, 0, -0.0000000039644384790685998654},
{0.000000003964416132809488937, 0, -0.000000035679745195285398778, 0, 0.00000014271898078114159511, 0, -0.00000033301095515599711703, 0, 0.00000049951643273399559613, 0, -0.00000049951643273399559613, 0, 0.00000033301095515599711703, 0, -0.00000014271898078114159511, 0, 0.000000035679745195285398778, 0, -0.000000003964416132809488937},
{0.0000000039642398349802689936, 0, -0.000000035678158514822424251, 0, 0.000000142712634059289697, 0, -0.00000033299614613834258222, 0, 0.00000049949421920751389981, 0, -0.00000049949421920751389981, 0, 0.00000033299614613834258222, 0, -0.000000142712634059289697, 0, 0.000000035678158514822424251, 0, -0.0000000039642398349802689936},
{0.0000000039643387068796392149, 0, -0.000000035679048361916743835, 0, 0.00000014271619344766697534, 0, -0.00000033300445137788969736, 0, 0.00000049950667706683451957, 0, -0.00000049950667706683451957, 0, 0.00000033300445137788969736, 0, -0.00000014271619344766697534, 0, 0.000000035679048361916743835, 0, -0.0000000039643387068796392149},
{0.0000000039643371694768541603, 0, -0.000000035679034525291679998, 0, 0.00000014271613810116671999, 0, -0.00000033300432223605573292, 0, 0.00000049950648335408354644, 0, -0.00000049950648335408354644, 0, 0.00000033300432223605573292, 0, -0.00000014271613810116671999, 0, 0.000000035679034525291679998, 0, -0.0000000039643371694768541603},
{0.0000000039642993043674485293, 0, -0.000000035678693739307041727, 0, 0.00000014271477495722816691, 0, -0.00000033300114156686566985, 0, 0.00000049950171235029853124, 0, -0.00000049950171235029853124, 0, 0.00000033300114156686566985, 0, -0.00000014271477495722816691, 0, 0.000000035678693739307041727, 0, -0.0000000039642993043674485293},
{0.0000000039643274817095983221, 0, -0.000000035678947335386385726, 0, 0.0000001427157893415455429, 0, -0.00000033300350846360622266, 0, 0.00000049950526269540936045, 0, -0.00000049950526269540936045, 0, 0.00000033300350846360622266, 0, -0.0000001427157893415455429, 0, 0.000000035678947335386385726, 0, -0.0000000039643274817095983221},
{0.0000000039643276407259596909, 0, -0.000000035678948766533630601, 0, 0.0000001427157950661345224, 0, -0.00000033300352182098058757, 0, 0.00000049950528273147080194, 0, -0.00000049950528273147080194, 0, 0.00000033300352182098058757, 0, -0.0000001427157950661345224, 0, 0.000000035678948766533630601, 0, -0.0000000039643276407259596909},
{0.0000000039643254219428611092, 0, -0.0000000356789287974857566, 0, 0.0000001427157151899430264, 0, -0.00000033300333544320041258, 0, 0.00000049950500316480061887, 0, -0.00000049950500316480061887, 0, 0.00000033300333544320041258, 0, -0.0000001427157151899430264, 0, 0.0000000356789287974857566, 0, -0.0000000039643254219428611092},
{0.0000000039643242128118546543, 0, -0.000000035678917915306682789, 0, 0.00000014271567166122673116, 0, -0.0000003330032338761957678, 0, 0.00000049950485081429359876, 0, -0.00000049950485081429359876, 0, 0.0000003330032338761957678, 0, -0.00000014271567166122673116, 0, 0.000000035678917915306682789, 0, -0.0000000039643242128118546543},
{0.0000000039643259488305705661, 0, -0.000000035678933539475138403, 0, 0.00000014271573415790055361, 0, -0.00000033300337970176802019, 0, 0.000000499505069552651845, 0, -0.000000499505069552651845, 0, 0.00000033300337970176802019, 0, -0.00000014271573415790055361, 0, 0.000000035678933539475138403, 0, -0.0000000039643259488305705661},
{0.0000000039643251768430161125, 0, -0.000000035678926591587140877, 0, 0.00000014271570636634856351, 0, -0.00000033300331485481335015, 0, 0.00000049950497228222002522, 0, -0.00000049950497228222002522, 0, 0.00000033300331485481335015, 0, -0.00000014271570636634856351, 0, 0.000000035678926591587140877, 0, -0.0000000039643251768430161125},
};

extern double filt_a[17][19] =
{
{1.0, -15.512893233900552303, 114.59676044638459302, -535.55562769965638381, 1774.1355117202765541, -4423.5219744932019239, 8602.4194547549086565, -13340.570563478609984, 16723.976420871844311, -17074.222917038219748, 14230.046453400507744, -9658.5255300136741425, 5299.4731705895010236, -2318.8118105074108826, 791.37661076555104955, -203.29153429436414058, 37.019550902405597981, -4.2651065262520937438, 0.23402383769767609589},
{1.0, -15.096623406904823739, 108.9290553468348719, -498.96540246658207707, 1625.50612828147041, -3998.4704640132958957, 7695.1454858460820105, -11845.6011195672545, 14784.390538415615083, -15071.903009228026349, 12579.820004700748541, -8576.327309428153967, 4740.6631576995696378, -2096.052238200553802, 725.09481791917949067, -189.40529375470077866, 35.188905620309718358, -4.1506575237977036963, 0.23402383769767562405},
{1.0, -14.620774104313241537, 102.63897704694885249, -459.35601354123400597, 1467.8918712142499317, -3555.1179192433673961, 6760.7344244056348543, -10319.657526618979318, 12815.163322487969708, -13042.579865261701343, 10904.358516583233722, -7471.6883032678379095, 4165.126584727620866, -1863.6966394831852085, 654.80505434560393496, -174.37314073067389586, 33.157236371213976156, -4.0198277723519755256, 0.23402383769767792776},
{1.0, -14.087223285928224925, 95.825723839025855, -417.66865307544424013, 1305.8757334799595355, -3107.8855117632901965, 5831.6008105323735435, -8817.6264115814828983, 10888.435162745101479, -11061.010289564012055, 9265.0381309001277259, -6384.3379810588312466, 3592.8262365896871415, -1629.3017856949709312, 582.55081520236467441, -158.55222235339221015, 30.956583792835992597, -3.8731335971733171775, 0.23402383769767548527},
{1.0, -13.498076633073802455, 88.596744796195480376, -374.86210543893491831, 1143.8747645876273964, -2670.0249965629936924, 4936.3934448110621815, -7386.6307076051598415, 9065.039354130582069, -9189.8516504374856595, 7713.6164819364057621, -5348.3877025022120506, 3041.4066744046340318, -1399.8119768446404123, 510.30165172988910172, -142.30636225122916017, 28.621653490943771203, -3.7111539331530658536, 0.23402383769767443056},
{1.0, -12.855659238433259972, 81.066045236345800618, -331.88082615840011158, 985.95341740095955174, -2253.0146199545743002, 4098.6887261043766557, -6063.9475038177315582, 7391.9031904354960716, -7477.0221829184611124, 6290.0214675132065167, -4390.8234816703243268, 2525.3895176469663966, -1181.2425189667430914, 439.86995460929767887, -125.99394796579089473, 26.189268711217891905, -3.5345280400236473994, 0.23402383769767548527},
{1.0, -12.162506429970665778, 73.352388790157490916, -289.62374148401374896, 835.66459551791751892, -1866.1184952196629183, 3336.1910323894962858, -4875.9516109065289129, 5900.9262306843647821, -5954.6357864746805717, 5021.3966106354100702, -3530.7410183665560908, 2055.680882392505282, -978.44882675692701923, 372.84008578250535493, -109.95609180809854877, 23.697789614670583092, -3.3439529794925051398, 0.2340238376976756518},
{1.0, -11.421353765149614645, 65.577424427701814125, -248.91486496543308249, 695.926868011855845, -1516.130075926022073, 2660.4562507606101462, -3838.0574018740398969, 4609.2479033018262271, -4639.4008054987971263, 3922.3283334664770337, -2779.3031791759799489, 1639.4009096576612592, -794.99196085269204559, 310.51363333388616184, -94.505481180692015641, 21.1865083149917659, -3.1401808642580788522, 0.23402383769767381994},
{1.0, -10.635126234937711587, 57.863767981513419159, -210.4767622453865954, 568.94398232082073719, -1207.3036452857595577, 2077.1026077255346536, -2955.5297655532995122, 3520.6756622677348787, -3534.2240796146970752, 2996.0605603320964292, -2140.3264982338005211, 1280.0133334377012488, -633.10265554842453639, 253.87352265057705836, -79.916309660920973101, 18.695029218444378927, -2.9240158897646146841, 0.23402383769767298727},
{1.0, -9.8069267202035401709, 50.333068421663853087, -174.90779361219171051, 456.17010983398972712, -941.46138827132699589, 1586.4297108285391005, -2224.9603889909712962, 2627.9511773393323892, -2630.6661333891815957, 2236.4223227476509237, -1611.3492604495813794, 977.70641687744569026, -493.7373275508172128, 203.56851510194428556, -66.415641562948039223, 16.262644438718751871, -2.696311160410109764, 0.23402383769767826083},
{1.0, -8.9400237460633391606, 43.104089378833371882, -142.6639292977344553, 358.3213722731298958, -718.24914877761523258, 1184.3366523887043513, -1636.1644632182935766, 1915.4921062275111581, -1911.8564159302136432, 1630.1600555124589391, -1185.003959731508985, 729.95807568980126234, -376.71197960338508892, 159.91833870330327727, -54.176511638763116707, 13.927714136826468305, -2.457965322732773128, 0.23402383769767595711},
{1.0, -8.0378385825057829805, 36.290836170910559133, -114.0457698340556334, 275.43130187199699321, -535.50271579562433999, 863.4156457946980936, -1174.2461804217064127, 1362.2586643700456079, -1355.49499157451055, 1159.3761830499988719, -850.5120412643337886, 532.2091497094204442, -280.89401526802458875, 122.93840459821780087, -43.312999851619522929, 11.727061558448781398, -2.2099190188641726351, 0.23402383769767501343},
{1.0, -7.1039317422022207893, 30.000757871024383405, -89.191218755178667266, 206.94516632374427445, -389.67941511463914139, 614.09925666634558183, -821.60490676674385213, 944.44802333199857003, -936.62732868923421847, 803.82135959005483983, -595.13583776419955029, 378.56975669715154709, -204.42827867925018381, 92.381849711732215269, -33.877450946394539244, 9.6953923093527425436, -1.9531511742440881285, 0.23402383769767531874},
{1.0, -6.1419889287898943309, 24.333052771474761755, -68.074054108455754886, 151.84571092438997653, -276.30746491888442051, 425.7550072392108973, -559.69973443754270193, 637.80135580709861642, -629.96553779145801855, 542.85728872834397407, -405.45575204989143003, 262.49333012296023071, -144.97241820217030295, 67.795585571074610698, -25.859932338778563121, 7.8647470272569819372, -1.6886751342491657102, 0.23402383769767701183},
{1.0, -5.1558064910834486838, 19.377103967404373464, -50.508435737877860561, 108.80098374015996399, -190.40806692991299087, 287.6473453721944793, -370.44829156633238654, 419.40215219150809389, -411.62513941977636023, 356.9868395166568007, -268.38282638182170103, 177.36907244816569573, -99.917988674310265651, 48.586192239555259675, -19.189944349056666795, 6.2639960818344242455, -1.4175346649816098488, 0.23402383769767320931},
{1.0, -4.1492764406206346095, 15.21106973199386303, -36.159174544457101774, 76.323632131355125807, -126.85215241676985443, 189.71887131923483594, -237.19620166738030775, 268.93528249176705458, -260.23578635190023078, 228.92545652975235271, -171.8606250963570119, 117.00318813533738194, -66.577345347569689693, 34.090926795304554275, -13.740316833752437731, 4.9183842717369472552, -1.1407998340013907335, 0.23402383769767484689},
{1.0, -3.1263710916707849741, 11.900650913803831443, -24.557385383914599686, 52.930469108916483378, -80.62425242553237581, 123.17506164105361677, -145.24806996601608944, 169.45223919735661866, -157.45625469092382787, 144.25248239765306835, -105.24978849985478746, 75.979168572222476996, -42.321907985899045457, 23.648852819264032377, -9.3331486532194851691, 3.849132699039162997, -0.8595627872582316531, 0.23402383769767551303},
};


#endif // BUTTER_COEFFICIENT_H_INCLUDED