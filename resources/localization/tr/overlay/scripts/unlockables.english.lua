unlockables = {

 -- ###############################################
 -- Unlock item and argument description
 -- ###############################################

 -- "unlock_rider"
 -- "unlock_visual"
 -- "unlock_track"
 -- "unlock_track_mirror"
 -- "unlock_track_reverse"
 -- "unlock_track_mirror_reverse"
 -- "unlock_videos"
 -- "unlock_livery"
 -- "unlock_arcade_class"
 -- "unlock_championship_class"

 -- ###############################################
 -- Unlock condition and argument description
 -- ###############################################

 -- "condition_reputation"
 -- "condition_track_first"
 -- "condition_track_podium"
 -- "condition_track_finish"
 -- "condition_season_125"
 -- "condition_season_250"
 -- "condition_season_GP"
 -- "condition_season_ALL"

 unlockable_rider_rossi = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda birer 125cc, 250cc ve MotoGP sezonu tamamlayın; her sezonu ilk iki sıradan birinde bitirin.]],
  unlock = "unlock_rider",
  unlock_arg = 0,
  condition = "condition_season_ALL",
  condition_arg = 1,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_stoner = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda bir MotoGP sezonu tamamlayın ve ilk iki sıradan birinde bitirin.]],
  unlock = "unlock_rider",
  unlock_arg = 4,
  condition = "condition_season_GP",
  condition_arg = 1,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_lorenzo = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda dört farklı MotoGP yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 1,
  condition = "condition_track_podium",
  condition_arg = 4,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_pedrosa = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda üç farklı MotoGP yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 2,
  condition = "condition_track_podium",
  condition_arg = 3,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_hayden = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda bir MotoGP yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 5,
  condition = "condition_track_podium",
  condition_arg = 1,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_edwards = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda iki farklı MotoGP yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 8,
  condition = "condition_track_podium",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_rider_simoncelli = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda üç farklı 250cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 58,
  condition = "condition_track_podium",
  condition_arg = 3,
  season = "bikes_2009",
  championship = "c250",
 };

 unlockable_rider_bautista = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda iki farklı 250cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 54,
  condition = "condition_track_podium",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "c250",
 };

 unlockable_rider_baldolini = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda bir 250cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 68,
  condition = "condition_track_podium",
  condition_arg = 1,
  season = "bikes_2009",
  championship = "c250",
 };

 unlockable_rider_smith = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda bir 125cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 21,
  condition = "condition_track_podium",
  condition_arg = 1,
  season = "bikes_2009",
  championship = "c125",
 };

 unlockable_rider_terol = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda iki farklı 125cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 30,
  condition = "condition_track_podium",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "c125",
 };

 unlockable_rider_cortese = {
  description = [[Şampiyona, Kariyer veya Arcade Modu'nda üç farklı 125cc yarışında podyuma çıkın.]],
  unlock = "unlock_rider",
  unlock_arg = 19,
  condition = "condition_track_podium",
  condition_arg = 3,
  season = "bikes_2009",
  championship = "c125",
 };

 unlockable_arcade_class_250 = {
  description = [[Herhangi bir oyun modunda bir 125cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_arcade_class",
  unlock_arg = 2,
  condition = "condition_season_125",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_arcade_class_GP2 = {
  description = [[Herhangi bir oyun modunda bir 125cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_arcade_class",
  unlock_arg = 3,
  condition = "condition_season_125",
  condition_arg = 2,
  season = "bikes_2010",
  championship = "gp",
 },

 unlockable_arcade_class_GP = {
  description = [[Herhangi bir oyun modunda bir 250cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_arcade_class",
  unlock_arg = 0,
  condition = "condition_season_250",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_championship_class_250 = {
  description = [[Herhangi bir oyun modunda bir 125cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_championship_class",
  unlock_arg = 2,
  condition = "condition_season_125",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "gp",
 },

 unlockable_championship_class_GP2 = {
  description = [[Herhangi bir oyun modunda bir 125cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_championship_class",
  unlock_arg = 3,
  condition = "condition_season_125",
  condition_arg = 2,
  season = "bikes_2010",
  championship = "gp",
 },

 unlockable_championship_class_GP = {
  description = [[Herhangi bir oyun modunda bir 250cc sezonu tamamlayın ve ilk üç sıradan birinde bitirin.]],
  unlock = "unlock_championship_class",
  unlock_arg = 0,
  condition = "condition_season_250",
  condition_arg = 2,
  season = "bikes_2009",
  championship = "gp",
 },
}
