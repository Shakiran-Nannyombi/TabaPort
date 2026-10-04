/**************************************************************************
 * TabaPort GEE 02: Sentinel-1 SAR flood history for recorded Kampala events
 * Change detection: post-event minimum vs pre-event median backscatter (VV).
 * Honest limit: a 6 to 12 day revisit and urban radar shadow mean flash
 * floods that drain within hours are often missed. What SAR does catch is
 * slow-draining valley water, which is exactly where culvert nodes belong.
 **************************************************************************/
var aoi = ee.Geometry.Rectangle([32.49, 0.22, 32.68, 0.41]);
Map.centerObject(aoi, 12);

// Recorded events (edit dates from the KCCA incident log when available)
var events = [
  {name: 'Mar2025', pre: ['2025-01-15', '2025-03-24'], post: ['2025-03-26', '2025-04-05']},  // GDACS event 1103187
  {name: 'Oct2025', pre: ['2025-08-15', '2025-10-29'], post: ['2025-10-31', '2025-11-10']},  // Nakivubo flooding 31 Oct 2025
  {name: 'Jul2026', pre: ['2026-05-15', '2026-07-01'], post: ['2026-07-02', '2026-07-31']}   // Nalukolongo, Lugogo, Kyambogo
];

var hand  = ee.Image('MERIT/Hydro/v1_0_1').select('hnd');
var slope = ee.Terrain.slope(ee.ImageCollection('COPERNICUS/DEM/GLO30')
  .filterBounds(aoi).select('DEM').mosaic()
  .setDefaultProjection({crs: 'EPSG:4326', scale: 30}));
var permWater = ee.Image('JRC/GSW1_4/GlobalSurfaceWater').select('occurrence').gt(80).unmask(0);

function s1(start, end) {
  return ee.ImageCollection('COPERNICUS/S1_GRD')
    .filterBounds(aoi).filterDate(start, end)
    .filter(ee.Filter.eq('instrumentMode', 'IW'))
    .filter(ee.Filter.listContains('transmitterReceiverPolarisation', 'VV'))
    .select('VV')
    .map(function (i) { return i.focalMedian(50, 'circle', 'meters'); }); // speckle
}

var floodMaps = events.map(function (e) {
  var preCol = s1(e.pre[0], e.pre[1]);
  var postCol = s1(e.post[0], e.post[1]);
  print(e.name + ' scenes (pre, post):', preCol.size(), postCol.size());
  var pre = preCol.median();
  var post = postCol.min();
  var flooded = post.subtract(pre).lt(-3)       // backscatter drop of 3 dB or more
    .and(post.lt(-15))                          // smooth open water
    .and(hand.lt(10))                           // physically plausible: near drainage
    .and(slope.lt(5))
    .and(permWater.eq(0))                       // not the lake
    .rename('flood').unmask(0).clip(aoi);
  Map.addLayer(flooded.selfMask(), {palette: ['0000ff']}, 'SAR flood ' + e.name, false);
  return flooded.set('event', e.name);
});

// Flood frequency: how many recorded events flooded each cell
var freq = ee.ImageCollection(floodMaps).sum().rename('flood_count').clip(aoi);
Map.addLayer(freq.selfMask(), {min: 1, max: events.length, palette: ['c6dbef', '2171b5', '08306b']},
  'Flood frequency (events)');

Export.image.toDrive({image: freq.toByte(), description: 'tabaport_sar_flood_frequency',
  folder: 'TabaPort_GIS', region: aoi, scale: 20, crs: 'EPSG:32636', maxPixels: 1e10});
