local CommonFunc = { 
setFeatureEnabled = function (this, path, status)
    local feature = this:getFeature(path)
    if (feature) then
        feature:setFeatureStatus(EffectSdk.BEF_FEATURE_STATUS_ENABLED, status)
    end
end,
} 
local FaceMakeupFunc = { 
playMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:show(type)
        feature_makeup:play(type)
        feature_makeup:seek(type, 0)
    end
end,

hideMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:pause(type)
        feature_makeup:hide(type)
    end
end,
} 
local Sticker2DV3 = { 
stopClip = function (this, path, entityName, clipName)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:resumeClip(entityName, clipName, false)
        feature_2dv3:appearClip(entityName, clipName, false)
    end
end,

playClip = function (this, path, entityName, clipName, playTimes)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:resetClip(entityName, clipName)
        feature_2dv3:playClip(entityName, clipName, -1, playTimes)
    end
end,

playLastClip = function (this, path, entityName, clipName)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:resumeClip(entityName, clipName, false)
        feature_2dv3:appearClip(entityName, clipName, true)
    end
end,
} 
local init_state = 1
local feature_0 = {
folder = "FaceMakeupV2_2999",
makeupType = { "mask2999", "pupil2998", "mask2997", "mask2996", "mask2995" }, 
}
local feature_1 = {
folder = "Filter_2998",
}
local feature_2 = {
folder = "FaceMakeupV2_2997",
makeupType = { "mask2997" }, 
}
EventHandles = {
    handleEffectEvent = function (this, eventCode)
        if (init_state == 1 and eventCode == 1) then
            init_state = 0
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[1])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[2])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[3])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[4])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[5])
            CommonFunc.setFeatureEnabled(this, feature_1.folder, true)
            FaceMakeupFunc.playMakeup(this, feature_2.folder, feature_2.makeupType[1])
        end
        return true
    end,
    handleRecodeVedioEvent = function (this, eventCode)
        if (eventCode == 1) then
            FaceMakeupFunc.hideMakeup(this, feature_0.folder, feature_0.makeupType[1])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[1])
            FaceMakeupFunc.hideMakeup(this, feature_0.folder, feature_0.makeupType[2])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[2])
            FaceMakeupFunc.hideMakeup(this, feature_0.folder, feature_0.makeupType[3])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[3])
            FaceMakeupFunc.hideMakeup(this, feature_0.folder, feature_0.makeupType[4])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[4])
            FaceMakeupFunc.hideMakeup(this, feature_0.folder, feature_0.makeupType[5])
            FaceMakeupFunc.playMakeup(this, feature_0.folder, feature_0.makeupType[5])
            CommonFunc.setFeatureEnabled(this, feature_1.folder, false)
            CommonFunc.setFeatureEnabled(this, feature_1.folder, true)
            FaceMakeupFunc.hideMakeup(this, feature_2.folder, feature_2.makeupType[1])
            FaceMakeupFunc.playMakeup(this, feature_2.folder, feature_2.makeupType[1])
        end
        return true
    end,
    }

