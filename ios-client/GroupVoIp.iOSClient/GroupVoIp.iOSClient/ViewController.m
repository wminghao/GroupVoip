//
//  ViewController.m
//  GroupVoIp.iOSClient
//
//  Created by Howard Wang on 4/8/14.
//  Copyright (c) 2014 Howard Wang. All rights reserved.
//

#import "ViewController.h"


@interface ViewController ()
{
    VLCMediaPlayer *_mediaplayer;
}
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    /* setup the media player instance, give it a delegate and something to draw into */
    _mediaplayer = [[VLCMediaPlayer alloc] init];
    _mediaplayer.delegate = self;
    _mediaplayer.drawable = self.movieView;
    
    /* create a media object and give it to the player */
    _mediaplayer.media = [VLCMedia mediaWithURL:[NSURL URLWithString:@"http://localhost/~wminghao/groupvoip/abc_32.flv"]];
}
- (IBAction)onPlay:(id)sender {
    
    if (_mediaplayer.isPlaying) {
        [_mediaplayer pause];
    }
    
    [_mediaplayer play];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
