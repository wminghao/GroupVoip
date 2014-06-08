//
//  ViewController.m
//  GroupVoIp.iOSClient
//
//  Created by Howard Wang on 4/8/14.
//  Copyright (c) 2014 Howard Wang. All rights reserved.
//

#import "ViewController.h"
#include <MobileVLCKit/MobileVLCKit.h>


@interface ViewController ()
{
    VLCMediaPlayer *_mediaplayer;
}
@property (weak, nonatomic) IBOutlet UITextField *ipAddrTextField;
@property (nonatomic) NSString* ipAddr;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    /* setup the media player instance, give it a delegate and something to draw into */
    _mediaplayer = [[VLCMediaPlayer alloc] init];
    _mediaplayer.delegate = self;
    _mediaplayer.drawable = self.movieView;
    
    _mediaplayer.media = nil;
    
    self.ipAddr = @"54.186.122.59";
    self.ipAddrTextField.text = self.ipAddr;
    self.ipAddrTextField.delegate = self;
    [self.ipAddrTextField addTarget:self
                  action:@selector(editingChanged:)
        forControlEvents:UIControlEventEditingChanged];
    
}
- (IBAction)onPlay:(id)sender {
    
    //http test
    //[NSString stringWithFormat:@"http://%@/~wminghao/groupvoip/abc_32.flv", self.ipAddr]
    //rtmp vod test
    //[NSString stringWithFormat:@"rtmp://%@/oflaDemo/abc_32", self.ipAddr]]
    //rtmp voip test
    //rtmp://%@/myRed5App/testliveA
    
    if( _mediaplayer.media == nil ) {
        /* create a media object and give it to the player */
        _mediaplayer.media = [VLCMedia mediaWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"rtmp://%@/myRed5App/__mixed__allinone", self.ipAddr]]];
    }
    
    if (_mediaplayer.isPlaying) {
        [_mediaplayer pause];
    }
    
    [_mediaplayer play];
}
- (IBAction)onStop:(id)sender {
    if( _mediaplayer.media != nil ) {
        [_mediaplayer stop];
    }
}
-(void) editingChanged:(id)sender {
    // your code
    self.ipAddr = self.ipAddrTextField.text;
}
-(BOOL) textFieldShouldReturn:(UITextField *)textField {
    self.ipAddr = self.ipAddrTextField.text;
    [textField resignFirstResponder];
    return YES;
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
