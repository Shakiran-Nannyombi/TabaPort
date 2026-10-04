/**************************************************************************
 * TabaPort GEE 01: Flood susceptibility, low points and blackspot check
 * Kampala, Uganda. IEEE Response Quest Concept #5267.
 * Workstream: GIS and Geomatics.
 * Output CRS for all exports: WGS 84 / UTM zone 36N (EPSG:32636).
 **************************************************************************/

// 1. AREA OF INTEREST ------------------------------------------------------
// Replace with the KCCA boundary asset when uploaded:
// var aoi = ee.FeatureCollection('projects/tabaport/assets/kcca_boundary').geometry();
var aoi = ee.Geometry.Rectangle([32.49, 0.22, 32.68, 0.41]);
Map.centerObject(aoi, 12);
Map.setOptions('HYBRID');

// 2. FLOOD LOCATION REGISTER (indicative points, to be replaced by GNSS fixes)
var register = ee.FeatureCollection([
  ee.Feature(ee.Geometry.Point([32.5785, 0.3095]), {id: 'TP-01', name: 'Clock Tower'}),
  ee.Feature(ee.Geometry.Point([32.5611, 0.3500]), {id: 'TP-02', name: 'Bwaise'}),
  ee.Feature(ee.Geometry.Point([32.5720, 0.3525]), {id: 'TP-03', name: 'Kalerwe'}),
  ee.Feature(ee.Geometry.Point([32.5745, 0.3395]), {id: 'TP-04', name: 'Mulago roundabout'}),
  ee.Feature(ee.Geometry.Point([32.5690, 0.3460]), {id: 'TP-05', name: 'Kubiri roundabout'}),
  ee.Feature(ee.Geometry.Point([32.5500, 0.3470]), {id: 'TP-06', name: 'Northern Bypass Lubigi'}),
  ee.Feature(ee.Geometry.Point([32.5775, 0.3110]), {id: 'TP-07', name: 'Nakivubo St Balikuddembe'}),
  ee.Feature(ee.Geometry.Point([32.5990, 0.3285]), {id: 'TP-08', name: 'Lugogo Bypass'}),
  ee.Feature(ee.Geometry.Point([32.5930, 0.3070]), {id: 'TP-09', name: 'Mukwano Rd'}),
  ee.Feature(ee.Geometry.Point([32.5350, 0.2983]), {id: 'TP-10', name: 'Nateete'}),
  ee.Feature(ee.Geometry.Point([32.5490, 0.2960]), {id: 'TP-11', name: 'Nalukolongo'}),
  ee.Feature(ee.Geometry.Point([32.5610, 0.2940]), {id: 'TP-12', name: 'Ndeeba'}),
  ee.Feature(ee.Geometry.Point([32.6330, 0.3350]), {id: 'TP-13', name: 'Kinawataka'}),
  ee.Feature(ee.Geometry.Point([32.6290, 0.3480]), {id: 'TP-14', name: 'Kyambogo'}),
  ee.Feature(ee.Geometry.Point([32.6144, 0.3550]), {id: 'TP-15', name: 'Ntinda'}),
  ee.Feature(ee.Geometry.Point([32.6078, 0.2872]), {id: 'TP-16', name: 'Kansanga'}),
  ee.Feature(ee.Geometry.Point([32.6530, 0.2890]), {id: 'TP-17', name: 'Port Bell'})
]);

// 3. TERRAIN AND HYDROLOGY ------------------------------------------------
var dem = ee.ImageCollection('COPERNICUS/DEM/GLO30')
  .filterBounds(aoi).select('DEM').mosaic()
  .setDefaultProjection({crs: 'EPSG:4326', scale: 30})
  .rename('elev').clip(aoi);

var slope = ee.Terrain.slope(dem).rename('slope');                // degrees
var merit = ee.Image('MERIT/Hydro/v1_0_1').clip(aoi);
var hand  = merit.select('hnd').rename('hand');                     // m above nearest drainage
var upa   = merit.select('upa');                                    // upstream area, km2

// Topographic Wetness Index: ln(a / tan b)
var sca = upa.multiply(1e6).divide(90);                             // m2 per m contour width
var slopeRad = ee.Terrain.slope(merit.select('elv'))
  .multiply(Math.PI / 180).max(0.001);
var twi = sca.log().subtract(slopeRad.tan().log()).rename('twi');

// Low-point index: elevation relative to the 300 m neighbourhood mean.
// Negative values = hollows and road dips where water ponds.
var localMean = dem.reduceNeighborhood({
  reducer: ee.Reducer.mean(), kernel: ee.Kernel.circle(300, 'meters')});
var tpi = dem.subtract(localMean).rename('tpi');

// Distance to drainage lines (cells draining at least 1 km2).
var streams = upa.gt(1).selfMask();
var distDrain = streams.fastDistanceTransform(100).sqrt()
  .multiply(90).rename('dist_drain')
  .reproject({crs: 'EPSG:32636', scale: 90});

// 4. SURFACE AND RAINFALL -------------------------------------------------
var built = ee.ImageCollection('GOOGLE/DYNAMICWORLD/V1')
  .filterBounds(aoi).filterDate('2025-01-01', '2026-01-01')
  .select('built').mean().rename('built').clip(aoi);                // 0 to 1

var years = ee.List.sequence(1995, 2025);
var rx1day = ee.ImageCollection(years.map(function (y) {
  y = ee.Number(y);
  return ee.ImageCollection('UCSB-CHG/CHIRPS/DAILY')
    .filter(ee.Filter.calendarRange(y, y, 'year'))
    .select('precipitation').max();
})).mean().rename('rx1day').clip(aoi);                              // mean annual max 1-day rain, mm

var permanentWater = ee.Image('JRC/GSW1_4/GlobalSurfaceWater')
  .select('occurrence').gt(80).unmask(0).clip(aoi);

// 5. NORMALISE (2nd to 98th percentile) ------------------------------------
function norm(img, invert) {
  var band = ee.String(img.bandNames().get(0));
  var p = img.reduceRegion({reducer: ee.Reducer.percentile([2, 98]),
    geometry: aoi, scale: 90, maxPixels: 1e10, bestEffort: true});
  var lo = ee.Number(p.get(band.cat('_p2')));
  var hi = ee.Number(p.get(band.cat('_p98')));
  var s = img.subtract(lo).divide(hi.subtract(lo)).clamp(0, 1);
  return invert ? ee.Image(1).subtract(s) : s;
}
var nHand  = norm(hand, true);       // low HAND  = high susceptibility
var nTpi   = norm(tpi, true);        // hollow    = high
var nTwi   = norm(twi, false);       // wet       = high
var nDist  = norm(distDrain, true);  // near drain = high
var nBuilt = norm(built, false);     // sealed    = high
var nSlope = norm(slope, true);      // flat      = high
var nRain  = norm(rx1day, false);    // wetter    = high

// 6. AHP WEIGHTS (CR = 0.007, computed in ahp.py) -------------------------
var fsi = nHand.multiply(0.316)
  .add(nTpi.multiply(0.197))
  .add(nTwi.multiply(0.184))
  .add(nDist.multiply(0.107))
  .add(nBuilt.multiply(0.107))
  .add(nSlope.multiply(0.057))
  .add(nRain.multiply(0.032))
  .where(permanentWater.eq(1), 1)
  .rename('fsi').clip(aoi);

// Five classes by quantile: 1 very low ... 5 very high
var q = fsi.reduceRegion({reducer: ee.Reducer.percentile([20, 40, 60, 80]),
  geometry: aoi, scale: 30, maxPixels: 1e10, bestEffort: true});
var fsiClass = ee.Image(1)
  .where(fsi.gt(ee.Number(q.get('fsi_p20'))), 2)
  .where(fsi.gt(ee.Number(q.get('fsi_p40'))), 3)
  .where(fsi.gt(ee.Number(q.get('fsi_p60'))), 4)
  .where(fsi.gt(ee.Number(q.get('fsi_p80'))), 5)
  .rename('fsi_class').clip(aoi);

// Low points: very high class AND within 5 m of drainage AND a local hollow
var lowPoints = fsiClass.eq(5).and(hand.lt(5)).and(tpi.lt(-1)).selfMask();

// 7. MAP ----------------------------------------------------------------
var pal = ['1a9850', '91cf60', 'fee08b', 'fc8d59', 'd73027'];
Map.addLayer(dem, {min: 1130, max: 1310, palette: ['08306b', 'f7fbff']}, 'Elevation (m)', false);
Map.addLayer(hand, {min: 0, max: 30, palette: ['08519c', 'deebf7']}, 'HAND (m)', false);
Map.addLayer(fsiClass, {min: 1, max: 5, palette: pal}, 'Flood susceptibility class');
Map.addLayer(lowPoints, {palette: ['7a0177']}, 'Low points (ponding)');
Map.addLayer(streams, {palette: ['00bfff']}, 'Drainage lines (>1 km2)', false);
Map.addLayer(register, {color: 'black'}, 'Flood location register');

// 8. VALIDATION: do documented blackspots fall in high classes? ------------
var atRegister = fsi.addBands(fsiClass).addBands(hand).addBands(tpi)
  .reduceRegions({collection: register.map(function (f) {
    return f.buffer(100); }),                                       // 100 m, same as IEEE text-report radius
  reducer: ee.Reducer.max(), scale: 30});
print('Register sampled (max within 100 m):', atRegister);

var background = fsi.sample({region: aoi, scale: 30, numPixels: 2000, seed: 42, geometries: false});
print('Mean FSI at register points:', atRegister.aggregate_mean('fsi'));
print('Mean FSI at 2000 random cells:', background.aggregate_mean('fsi'));
print('Share of register in class 4 or 5:',
  atRegister.filter(ee.Filter.gte('fsi_class', 4)).size()
    .divide(atRegister.size()));

// 9. EXPORTS (feed Sharif's offline Leaflet stack) ---------------------------
Export.image.toDrive({image: fsi.addBands(fsiClass).addBands(hand).float(),
  description: 'tabaport_fsi_hand_30m', folder: 'TabaPort_GIS',
  region: aoi, scale: 30, crs: 'EPSG:32636', maxPixels: 1e10});

var lowPolys = lowPoints.reduceToVectors({geometry: aoi, scale: 30,
  geometryType: 'polygon', eightConnected: true, maxPixels: 1e10})
  .map(function (f) { return f.set('area_m2', f.geometry().area(1)); })
  .filter(ee.Filter.gt('area_m2', 2000));                           // drop specks under 0.2 ha
Export.table.toDrive({collection: lowPolys, description: 'tabaport_low_points',
  folder: 'TabaPort_GIS', fileFormat: 'GeoJSON'});

Export.table.toDrive({collection: atRegister, description: 'tabaport_register_check',
  folder: 'TabaPort_GIS', fileFormat: 'CSV'});
